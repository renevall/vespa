// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.hosted.controller.api.integration.dns;

import com.yahoo.config.provision.HostName;

import java.util.Objects;

/**
 * The target of an {@link Record.Type#ALIAS} record. Contains record fields unique to aliases.
 *
 * @author mpolden
 */
public abstract class AliasTarget {

    private final HostName name;
    private final String dnsZone;
    private final String id;

    public AliasTarget(HostName name, String dnsZone, String id) {
        this.name = Objects.requireNonNull(name, "name must be non-null");
        this.dnsZone = Objects.requireNonNull(dnsZone, "dnsZone must be non-null");
        this.id = Objects.requireNonNull(id, "id must be non-null");
    }

    /** A unique identifier of this record within the ALIAS record group */
    public String id() {
        return id;
    }

    /** DNS name this points to */
    public final HostName name() {
        return name;
    }

    /** The DNS zone this belongs to */
    public final String dnsZone() {
        return dnsZone;
    }

    /** Returns the fields in this encoded as record data */
    public abstract RecordData pack();

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        AliasTarget alias = (AliasTarget) o;
        return name.equals(alias.name) &&
               dnsZone.equals(alias.dnsZone) &&
               id.equals(alias.id);
    }

    @Override
    public int hashCode() {
        return Objects.hash(name, dnsZone, id);
    }

    /** Unpack target from given record data */
    public static AliasTarget unpack(RecordData data) {
        String[] parts = data.asString().split("/");
        switch (parts[0]) {
            case "latency": return LatencyAliasTarget.unpack(data);
            case "weighted": return WeightedAliasTarget.unpack(data);
        }
        throw new IllegalArgumentException("Unknown alias type '" + parts[0] + "'");
    }

}
