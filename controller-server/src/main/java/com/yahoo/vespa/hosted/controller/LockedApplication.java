// Copyright 2018 Yahoo Holdings. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.hosted.controller;

import com.yahoo.config.application.api.DeploymentSpec;
import com.yahoo.config.application.api.ValidationOverrides;
import com.yahoo.config.provision.InstanceName;
import com.yahoo.vespa.curator.Lock;
import com.yahoo.vespa.hosted.controller.api.integration.organization.IssueId;
import com.yahoo.vespa.hosted.controller.api.integration.organization.User;
import com.yahoo.vespa.hosted.controller.application.Change;
import com.yahoo.vespa.hosted.controller.application.TenantAndApplicationId;
import com.yahoo.vespa.hosted.controller.metric.ApplicationMetrics;

import java.time.Instant;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.OptionalInt;
import java.util.OptionalLong;
import java.util.Set;
import java.util.function.UnaryOperator;

/**
 * An application that has been locked for modification. Provides methods for modifying an application's fields.
 *
 * @author jonmv
 */
public class LockedApplication {

    private final Lock lock;
    private final TenantAndApplicationId id;
    private final Instant createdAt;
    private final DeploymentSpec deploymentSpec;
    private final ValidationOverrides validationOverrides;
    private final Change change;
    private final Change outstandingChange;
    private final Optional<IssueId> deploymentIssueId;
    private final Optional<IssueId> ownershipIssueId;
    private final Optional<User> owner;
    private final OptionalInt majorVersion;
    private final ApplicationMetrics metrics;
    private final List<String> pemDeployKeys;
    private final OptionalLong projectId;
    private final boolean internal;
    private final Map<InstanceName, Instance> instances;

    /**
     * Used to create a locked application
     *
     * @param application The application to lock.
     * @param lock The lock for the application.
     */
    LockedApplication(Application application, Lock lock) {
        this(Objects.requireNonNull(lock, "lock cannot be null"), application.id(), application.createdAt(),
             application.deploymentSpec(), application.validationOverrides(), application.change(),
             application.outstandingChange(), application.deploymentIssueId(), application.ownershipIssueId(),
             application.owner(), application.majorVersion(), application.metrics(), application.pemDeployKeys(),
             application.projectId(), application.internal(), application.instances());
    }

    private LockedApplication(Lock lock, TenantAndApplicationId id, Instant createdAt, DeploymentSpec deploymentSpec,
                              ValidationOverrides validationOverrides, Change change, Change outstandingChange,
                              Optional<IssueId> deploymentIssueId, Optional<IssueId> ownershipIssueId, Optional<User> owner,
                              OptionalInt majorVersion, ApplicationMetrics metrics, List<String> pemDeployKeys,
                              OptionalLong projectId, boolean internal, Map<InstanceName, Instance> instances) {
        this.lock = lock;
        this.id = id;
        this.createdAt = createdAt;
        this.deploymentSpec = deploymentSpec;
        this.validationOverrides = validationOverrides;
        this.change = change;
        this.outstandingChange = outstandingChange;
        this.deploymentIssueId = deploymentIssueId;
        this.ownershipIssueId = ownershipIssueId;
        this.owner = owner;
        this.majorVersion = majorVersion;
        this.metrics = metrics;
        this.pemDeployKeys = pemDeployKeys;
        this.projectId = projectId;
        this.internal = internal;
        this.instances = Map.copyOf(instances);
    }

    /** Returns a read-only copy of this */
    public Application get() {
        return new Application(id, createdAt, deploymentSpec, validationOverrides, change, outstandingChange,
                               deploymentIssueId, ownershipIssueId, owner, majorVersion, metrics, pemDeployKeys,
                               projectId, internal, instances.values());
    }

    public LockedApplication withNewInstance(InstanceName instance) {
        var instances = new HashMap<>(this.instances);
        instances.put(instance, new Instance(id.instance(instance)));
        return new LockedApplication(lock, id, createdAt, deploymentSpec, validationOverrides, change, outstandingChange,
                                     deploymentIssueId, ownershipIssueId, owner, majorVersion, metrics, pemDeployKeys,
                                     projectId, internal, instances);
    }

    public LockedApplication with(InstanceName instance, UnaryOperator<Instance> modification) {
        var instances = new HashMap<>(this.instances);
        instances.put(instance, modification.apply(instances.get(instance)));
        return new LockedApplication(lock, id, createdAt, deploymentSpec, validationOverrides, change, outstandingChange,
                                     deploymentIssueId, ownershipIssueId, owner, majorVersion, metrics, pemDeployKeys,
                                     projectId, internal, instances);
    }

    public LockedApplication without(InstanceName instance) {
        var instances = new HashMap<>(this.instances);
        instances.remove(instance);
        return new LockedApplication(lock, id, createdAt, deploymentSpec, validationOverrides, change, outstandingChange,
                                     deploymentIssueId, ownershipIssueId, owner, majorVersion, metrics, pemDeployKeys,
                                     projectId, internal, instances);
    }

    public LockedApplication withBuiltInternally(boolean builtInternally) {
        return new LockedApplication(lock, id, createdAt, deploymentSpec, validationOverrides, change, outstandingChange,
                                     deploymentIssueId, ownershipIssueId, owner, majorVersion, metrics, pemDeployKeys,
                                     projectId, builtInternally, instances);
    }

    public LockedApplication withProjectId(OptionalLong projectId) {
        return new LockedApplication(lock, id, createdAt, deploymentSpec, validationOverrides, change, outstandingChange,
                                     deploymentIssueId, ownershipIssueId, owner, majorVersion, metrics, pemDeployKeys,
                                     projectId, internal, instances);
    }

    public LockedApplication withDeploymentIssueId(IssueId issueId) {
        return new LockedApplication(lock, id, createdAt, deploymentSpec, validationOverrides, change, outstandingChange,
                                     Optional.ofNullable(issueId), ownershipIssueId, owner, majorVersion, metrics, pemDeployKeys,
                                     projectId, internal, instances);
    }

    public LockedApplication with(DeploymentSpec deploymentSpec) {
        return new LockedApplication(lock, id, createdAt, deploymentSpec, validationOverrides, change, outstandingChange,
                                     deploymentIssueId, ownershipIssueId, owner, majorVersion, metrics, pemDeployKeys,
                                     projectId, internal, instances);
    }

    public LockedApplication with(ValidationOverrides validationOverrides) {
        return new LockedApplication(lock, id, createdAt, deploymentSpec, validationOverrides, change, outstandingChange,
                                     deploymentIssueId, ownershipIssueId, owner, majorVersion, metrics, pemDeployKeys,
                                     projectId, internal, instances);
    }

    public LockedApplication withChange(Change change) {
        return new LockedApplication(lock, id, createdAt, deploymentSpec, validationOverrides, change, outstandingChange,
                                     deploymentIssueId, ownershipIssueId, owner, majorVersion, metrics, pemDeployKeys,
                                     projectId, internal, instances);
    }

    public LockedApplication withOutstandingChange(Change outstandingChange) {
        return new LockedApplication(lock, id, createdAt, deploymentSpec, validationOverrides, change, outstandingChange,
                                     deploymentIssueId, ownershipIssueId, owner, majorVersion, metrics, pemDeployKeys,
                                     projectId, internal, instances);
    }

    public LockedApplication withOwnershipIssueId(IssueId issueId) {
        return new LockedApplication(lock, id, createdAt, deploymentSpec, validationOverrides, change, outstandingChange,
                                     deploymentIssueId, Optional.of(issueId), owner, majorVersion, metrics, pemDeployKeys,
                                     projectId, internal, instances);
    }

    public LockedApplication withOwner(User owner) {
        return new LockedApplication(lock, id, createdAt, deploymentSpec, validationOverrides, change, outstandingChange,
                                     deploymentIssueId, ownershipIssueId, Optional.of(owner), majorVersion, metrics, pemDeployKeys,
                                     projectId, internal, instances);
    }

    /** Set a major version for this, or set to null to remove any major version override */
    public LockedApplication withMajorVersion(Integer majorVersion) {
        return new LockedApplication(lock, id, createdAt, deploymentSpec, validationOverrides, change, outstandingChange,
                                     deploymentIssueId, ownershipIssueId, owner,
                                     majorVersion == null ? OptionalInt.empty() : OptionalInt.of(majorVersion),
                                     metrics, pemDeployKeys, projectId, internal, instances);
    }

    public LockedApplication with(ApplicationMetrics metrics) {
        return new LockedApplication(lock, id, createdAt, deploymentSpec, validationOverrides, change, outstandingChange,
                                     deploymentIssueId, ownershipIssueId, owner, majorVersion, metrics, pemDeployKeys,
                                     projectId, internal, instances);
    }

    public LockedApplication withPemDeployKey(String pemDeployKey) {
        List<String> keys = new ArrayList<>(pemDeployKeys);
        keys.remove(pemDeployKey);
        keys.add(pemDeployKey);
        return new LockedApplication(lock, id, createdAt, deploymentSpec, validationOverrides, change, outstandingChange,
                                     deploymentIssueId, ownershipIssueId, owner, majorVersion, metrics, keys,
                                     projectId, internal, instances);
    }

    public LockedApplication withoutPemDeployKey(String pemDeployKey) {
        List<String> keys = new ArrayList<>(pemDeployKeys);
        keys.remove(pemDeployKey);
        return new LockedApplication(lock, id, createdAt, deploymentSpec, validationOverrides, change, outstandingChange,
                                     deploymentIssueId, ownershipIssueId, owner, majorVersion, metrics, keys,
                                     projectId, internal, instances);
    }

    @Override
    public String toString() {
        return "application '" + id + "'";
    }

}
