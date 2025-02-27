// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.data.access.slime;


import java.util.Map;
import java.util.AbstractMap;
import java.util.List;
import java.util.ArrayList;


public final class SlimeAdapter implements com.yahoo.data.access.Inspector {
    private com.yahoo.slime.Inspector inspector;
    public SlimeAdapter(com.yahoo.slime.Inspector inspector) { this.inspector = inspector; }
    @Override public boolean equals(Object rhs) {
        if (!(rhs instanceof SlimeAdapter)) {
            return false;
        }
        return inspector.equals(((SlimeAdapter)rhs).inspector);
    }
    @Override public int hashCode() { return inspector.hashCode(); }
    @Override public String toString() { return inspector.toString(); }
    public com.yahoo.data.access.Inspector inspect() { return this; }
    public boolean valid() { return inspector.valid(); }
    public com.yahoo.data.access.Type type() {
        switch(inspector.type()) {
        case NIX:    return com.yahoo.data.access.Type.EMPTY;
        case BOOL:   return com.yahoo.data.access.Type.BOOL;
        case LONG:   return com.yahoo.data.access.Type.LONG;
        case DOUBLE: return com.yahoo.data.access.Type.DOUBLE;
        case STRING: return com.yahoo.data.access.Type.STRING;
        case DATA:   return com.yahoo.data.access.Type.DATA;
        case ARRAY:  return com.yahoo.data.access.Type.ARRAY;
        case OBJECT: return com.yahoo.data.access.Type.OBJECT;
        }
        return com.yahoo.data.access.Type.EMPTY;
    }
    private boolean verify(com.yahoo.slime.Type ok_type_a) {
        com.yahoo.slime.Type my_type = inspector.type();
        return (valid() && (my_type == ok_type_a));
    }
    private boolean verify(com.yahoo.slime.Type ok_type_a,
                           com.yahoo.slime.Type ok_type_b)
    {
        com.yahoo.slime.Type my_type = inspector.type();
        return (valid() && (my_type == ok_type_a || my_type == ok_type_b));
    }
    private boolean verify(com.yahoo.slime.Type ok_type_a,
                           com.yahoo.slime.Type ok_type_b,
                           com.yahoo.slime.Type ok_type_c)
    {
        com.yahoo.slime.Type my_type = inspector.type();
        return (valid() && (my_type == ok_type_a || my_type == ok_type_b || my_type == ok_type_c));
    }
    public int entryCount() { return inspector.entries();  }
    public int fieldCount() { return inspector.fields();   }
    public boolean asBool() {
        if (!verify(com.yahoo.slime.Type.NIX, com.yahoo.slime.Type.BOOL)) {
            throw new IllegalStateException("invalid data extraction!");
        }
        return inspector.asBool();
    }
    public long asLong() {
        if (!verify(com.yahoo.slime.Type.NIX, com.yahoo.slime.Type.LONG, com.yahoo.slime.Type.DOUBLE)) {
            throw new IllegalStateException("invalid data extraction!");
        }
        return inspector.asLong();
    }
    public double asDouble() {
        if (!verify(com.yahoo.slime.Type.NIX, com.yahoo.slime.Type.DOUBLE, com.yahoo.slime.Type.LONG)) {
            throw new IllegalStateException("invalid data extraction!");
        }
        return inspector.asDouble();
    }
    public String asString() {
        if (!verify(com.yahoo.slime.Type.NIX, com.yahoo.slime.Type.STRING)) {
            throw new IllegalStateException("invalid data extraction!");
        }
        return inspector.asString();
    }
    public byte[] asUtf8() {
        if (!verify(com.yahoo.slime.Type.NIX, com.yahoo.slime.Type.STRING)) {
            throw new IllegalStateException("invalid data extraction!");
        }
        return inspector.asUtf8();
    }
    public byte[] asData() {
        if (!verify(com.yahoo.slime.Type.NIX, com.yahoo.slime.Type.DATA)) {
            throw new IllegalStateException("invalid data extraction!");
        }
        return inspector.asData();
    }
    public boolean asBool(boolean defaultValue) {
        if (!verify(com.yahoo.slime.Type.BOOL)) {
            return defaultValue;
        }
        return inspector.asBool();
    }
    public long asLong(long defaultValue) {
        if (!verify(com.yahoo.slime.Type.LONG, com.yahoo.slime.Type.DOUBLE)) {
            return defaultValue;
        }
        return inspector.asLong();
    }
    public double asDouble(double defaultValue) {
        if (!verify(com.yahoo.slime.Type.DOUBLE, com.yahoo.slime.Type.LONG)) {
            return defaultValue;
        }
        return inspector.asDouble();
    }
    public String asString(String defaultValue) {
        if (!verify(com.yahoo.slime.Type.STRING)) {
            return defaultValue;
        }
        return inspector.asString();
    }
    public byte[] asUtf8(byte[] defaultValue) {
        if (!verify(com.yahoo.slime.Type.STRING)) {
            return defaultValue;
        }
        return inspector.asUtf8();
    }
    public byte[] asData(byte[] defaultValue) {
        if (!verify(com.yahoo.slime.Type.DATA)) {
            return defaultValue;
        }
        return inspector.asData();
    }
    public void traverse(final com.yahoo.data.access.ArrayTraverser at) {
        inspector.traverse(new com.yahoo.slime.ArrayTraverser() {
                public void entry(int idx, com.yahoo.slime.Inspector inspector) { at.entry(idx, new SlimeAdapter(inspector)); }
            });
    }
    public void traverse(final com.yahoo.data.access.ObjectTraverser ot) {
        inspector.traverse(new com.yahoo.slime.ObjectTraverser() {
                public void field(String name, com.yahoo.slime.Inspector inspector) { ot.field(name, new SlimeAdapter(inspector)); }
            });
    }
    public com.yahoo.data.access.Inspector entry(int idx) { return new SlimeAdapter(inspector.entry(idx)); }
    public com.yahoo.data.access.Inspector field(String name) { return new SlimeAdapter(inspector.field(name)); }
    public Iterable<com.yahoo.data.access.Inspector> entries() {
        final List<com.yahoo.data.access.Inspector> list = new ArrayList<>();
        inspector.traverse(new com.yahoo.slime.ArrayTraverser() {
                public void entry(int idx, com.yahoo.slime.Inspector inspector) { list.add(new SlimeAdapter(inspector)); }
            });
        return list;
    }
    public Iterable<Map.Entry<String,com.yahoo.data.access.Inspector>> fields() {
        final List<Map.Entry<String,com.yahoo.data.access.Inspector>> list = new ArrayList<>();
        inspector.traverse(new com.yahoo.slime.ObjectTraverser() {
                public void field(String name, com.yahoo.slime.Inspector inspector) {
                    list.add(new AbstractMap.SimpleImmutableEntry<String,com.yahoo.data.access.Inspector>(name, new SlimeAdapter(inspector)));
                }
            });
        return list;
    }
}
