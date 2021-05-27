// Copyright Verizon Media. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.hosted.provision.os;

import com.yahoo.component.Version;
import com.yahoo.config.provision.NodeType;
import com.yahoo.vespa.flags.IntFlag;
import com.yahoo.vespa.flags.PermanentFlags;
import com.yahoo.vespa.hosted.provision.Node;
import com.yahoo.vespa.hosted.provision.NodeList;
import com.yahoo.vespa.hosted.provision.NodeRepository;
import com.yahoo.vespa.hosted.provision.node.Agent;
import com.yahoo.vespa.hosted.provision.node.ClusterId;
import com.yahoo.vespa.hosted.provision.node.filter.NodeListFilter;

import java.time.Instant;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.logging.Logger;

/**
 * An upgrader that retires and rebuilds hosts on stale OS versions.
 *
 * - We limit the number of concurrent rebuilds to reduce impact of retiring too many hosts.
 * - We limit rebuilds by cluster so that at most one node per stateful cluster per application is retired at a time.
 *
 * Used in cases where performing an OS upgrade requires rebuilding the host, e.g. when upgrading across major versions.
 *
 * @author mpolden
 */
public class RebuildingOsUpgrader implements OsUpgrader {

    private static final Logger LOG = Logger.getLogger(RebuildingOsUpgrader.class.getName());

    private final NodeRepository nodeRepository;
    private final IntFlag maxRebuilds;

    public RebuildingOsUpgrader(NodeRepository nodeRepository) {
        this.nodeRepository = nodeRepository;
        this.maxRebuilds = PermanentFlags.MAX_REBUILDS.bindTo(nodeRepository.flagSource());
    }

    @Override
    public void upgradeTo(OsVersionTarget target) {
        NodeList allNodes = nodeRepository.nodes().list();
        Instant now = nodeRepository.clock().instant();
        rebuildableHosts(target, allNodes).forEach(host -> rebuild(host, target.version(), now));
    }

    @Override
    public void disableUpgrade(NodeType type) {
        // No action needed in this implementation. Hosts that have started rebuilding cannot be halted
    }

    /** Returns the number of hosts of given type that can be rebuilt concurrently */
    private int rebuildLimit(NodeType hostType, NodeList hostsOfType) {
        if (hostsOfType.stream().anyMatch(host -> host.type() != hostType)) illegal("All hosts must be a " + hostType);
        int limit = hostType == NodeType.host ? maxRebuilds.value() : 1;
        return Math.max(0, limit - hostsOfType.rebuilding().size());
    }

    private List<Node> rebuildableHosts(OsVersionTarget target, NodeList allNodes) {
        NodeList hostsOfTargetType = allNodes.nodeType(target.nodeType());
        int rebuildLimit = rebuildLimit(target.nodeType(), hostsOfTargetType);

        // Find stateful clusters with retiring nodes
        NodeList activeNodes = allNodes.state(Node.State.active);
        Set<ClusterId> retiringClusters = new HashSet<>(activeNodes.nodeType(target.nodeType().childNodeType())
                                                                   .retiring().statefulClusters());

        // Rebuild hosts not containing stateful clusters with retiring nodes, up to rebuild limit
        List<Node> hostsToRebuild = new ArrayList<>(rebuildLimit);
        NodeList candidates = hostsOfTargetType.state(Node.State.active)
                                               .not().rebuilding()
                                               .osVersionIsBefore(target.version())
                                               .byIncreasingOsVersion();
        for (Node host : candidates) {
            if (hostsToRebuild.size() == rebuildLimit) break;
            Set<ClusterId> clustersOnHost = activeNodes.childrenOf(host).statefulClusters();
            boolean canRebuild = Collections.disjoint(retiringClusters, clustersOnHost);
            if (canRebuild) {
                hostsToRebuild.add(host);
                retiringClusters.addAll(clustersOnHost);
            }
        }
        return Collections.unmodifiableList(hostsToRebuild);
    }

    private void rebuild(Node host, Version target, Instant now) {
        LOG.info("Retiring and rebuilding " + host + ": On stale OS version " +
                 host.status().osVersion().current().map(Version::toFullString).orElse("<unset>") +
                 ", want " + target);
        nodeRepository.nodes().rebuild(host.hostname(), Agent.RebuildingOsUpgrader, now);
        nodeRepository.nodes().upgradeOs(NodeListFilter.from(host), Optional.of(target));
    }

    private static void illegal(String msg) {
        throw new IllegalArgumentException(msg);
    }

}
