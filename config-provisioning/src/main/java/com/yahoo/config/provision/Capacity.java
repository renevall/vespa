// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.config.provision;

/**
 * A capacity request.
 *
 * @author Ulf Lilleengen
 * @author bratseth
 */
public final class Capacity {

    /** Resources should stay between these values, inclusive */
    private final ClusterResources min, max;

    private final boolean required;

    private final boolean canFail;

    private final NodeType type;

    private Capacity(ClusterResources min, ClusterResources max, boolean required, boolean canFail, NodeType type) {
        validate(min);
        validate(max);
        if (max.smallerThan(min))
            throw new IllegalArgumentException("The max capacity must be larger than the min capacity, but got min " +
                                               min + " and max " + max);
        this.min = min;
        this.max = max;
        this.required = required;
        this.canFail = canFail;
        this.type = type;
    }

    private static void validate(ClusterResources resources) {
        if (resources.nodes() == 0 && resources.groups() == 0) return; // unspecified
        if (resources.nodes() % resources.groups() != 0)
            throw new IllegalArgumentException("The number of nodes (" + resources.nodes() +
                                               ") must be divisible by the number of groups (" + resources.groups() + ")");
    }

    public ClusterResources minResources() { return min; }
    public ClusterResources maxResources() { return max; }

    /** Returns whether the requested number of nodes must be met exactly for a request for this to succeed */
    public boolean isRequired() { return required; }

    /**
     * Returns true if an exception should be thrown if the specified capacity can not be satisfied
     * (to whatever policies are applied and taking required true/false into account).
     * Returns false if it is preferable to still succeed with partially satisfied capacity.
     */
    public boolean canFail() { return canFail; }

    /**
     * Returns the node type (role) requested. This is tenant nodes by default.
     * If some other type is requested the node count and flavor may be ignored
     * and all nodes of the requested type returned instead.
     */
    public NodeType type() { return type; }

    public Capacity withGroups(int groups) {
        return new Capacity(min.withGroups(groups), max.withGroups(groups), required, canFail, type);
    }

    @Override
    public String toString() {
        return (required ? "required " : "") +
               (min.equals(max) ? min : "between " + min + " and " + max);
    }

    /** Create a non-required, failable capacity request */
    public static Capacity from(ClusterResources resources) {
        return from(resources, resources);
    }

    /** Create a non-required, failable capacity request */
    public static Capacity from(ClusterResources min, ClusterResources max) {
        return from(min, max, false, true);
    }

    public static Capacity from(ClusterResources resources, boolean required, boolean canFail) {
        return from(resources, required, canFail, NodeType.tenant);
    }

    public static Capacity from(ClusterResources min, ClusterResources max, boolean required, boolean canFail) {
        return new Capacity(min, max, required, canFail, NodeType.tenant);
    }

    /** Creates this from a node type */
    public static Capacity fromRequiredNodeType(NodeType type) {
        return from(new ClusterResources(0, 0, NodeResources.unspecified()), true, false, type);
    }

    private static Capacity from(ClusterResources resources, boolean required, boolean canFail, NodeType type) {
        return new Capacity(resources, resources, required, canFail, type);
    }

}
