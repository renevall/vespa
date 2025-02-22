// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.slime;

/**
 * A value holding a String in Java native format.
 * See also @ref Utf8Value (for lazy decoding).
 **/
final class StringValue extends Value {
    private final String value;
    private byte[] utf8;
    private StringValue(String value) { this.value = value; }
    public static Value create(String value) {
        if (value == null) {
            return NixValue.instance();
        } else {
            return new StringValue(value);
        }
    }
    public final Type type() { return Type.STRING; }
    public final String asString() { return this.value; }
    public final byte[] asUtf8() {
        if (utf8 == null) {
            utf8 = Utf8Codec.encode(value);
        }
        return utf8;
    }
    public final void accept(Visitor v) { v.visitString(value); }
}
