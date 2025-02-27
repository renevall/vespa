// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.slime;

/**
 * A Value holding a slime "Object", a dynamic collection of named
 * value fields.  Fields can be inspected or traversed using the
 * {@link Inspector} interface, and you can add new fields by using the
 * various "set" methods in the @ref Cursor interface.
 **/
final class ObjectValue extends Value {

    private int capacity = 16;
    private int hashSize() { return (capacity + (capacity >> 1) - 1); }
    private int used = 0;
    private Value[] values = new Value[capacity];
    private int[] hash = new int[capacity + hashSize() + (capacity << 1)];
    private final SymbolTable names;

    private final void rehash() {
        capacity = (capacity << 1);
        Value[] v = values;
        values = new Value[capacity];
        System.arraycopy(v, 0, values, 0, used);
        int[] h = hash;
        hash = new int[capacity + hashSize() + (capacity << 1)];
        System.arraycopy(h, 0, hash, 0, used);
        for (int i = 0; i < used; i++) {
            int prev = (capacity + (hash[i] % hashSize()));
            int entry = hash[prev];
            while (entry != 0) {
                prev = entry + 1;
                entry = hash[prev];
            }
            final int insertIdx = (capacity + hashSize() + (i << 1));
            hash[prev] = insertIdx;
            hash[insertIdx] = i;
        }
    }

    private final Value put(int sym, Value value) {
        if (used == capacity) {
            rehash();
        }
        int prev = (capacity + (sym % hashSize()));
        int entry = hash[prev];
        while (entry != 0) {
            final int idx = hash[entry];
            if (hash[idx] == sym) { // found entry
                return NixValue.invalid();
            }
            prev = entry + 1;
            entry = hash[prev];
        }
        final int insertIdx = (capacity + hashSize() + (used << 1));
        hash[prev] = insertIdx;
        hash[insertIdx] = used;
        hash[used] = sym;
        values[used++] = value;
        return value;
    }

    private final Value get(int sym) {
        int entry = hash[capacity + (sym % hashSize())];
        while (entry != 0) {
            final int idx = hash[entry];
            if (hash[idx] == sym) { // found entry
                return values[idx];
            }
            entry = hash[entry + 1];
        }
        return NixValue.invalid();
    }

    public ObjectValue(SymbolTable names) { this.names = names; }
    public ObjectValue(SymbolTable names, int sym, Value value) {
        this.names = names;
        put(sym, value);
    }

    public final Type type() { return Type.OBJECT; }
    public final int children() { return used; }
    public final int fields() { return used; }

    public final Value field(int sym) { return get(sym); }
    public final Value field(String name) { return get(names.lookup(name)); }

    public final void accept(Visitor v) { v.visitObject(this); }

    public final void traverse(ObjectSymbolTraverser ot) {
        for (int i = 0; i < used; ++i) {
            ot.field(hash[i], values[i]);
        }
    }

    public final void traverse(ObjectTraverser ot) {
        for (int i = 0; i < used; ++i) {
            ot.field(names.inspect(hash[i]), values[i]);
        }
    }

    protected final Cursor setLeaf(int sym, Value value) { return put(sym, value); }
    public final Cursor setArray(int sym) { return put(sym, new ArrayValue(names)); }
    public final Cursor setObject(int sym) { return put(sym, new ObjectValue(names)); }

    protected final Cursor setLeaf(String name, Value value) { return put(names.insert(name), value); }
    public final Cursor setArray(String name) { return put(names.insert(name), new ArrayValue(names)); }
    public final Cursor setObject(String name) { return put(names.insert(name), new ObjectValue(names)); }
}
