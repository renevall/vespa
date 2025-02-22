// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.prelude.query;

/**
 * Used to signal that this term requires exact match if the backend supports it.
 *
 * @author baldersheim
 */

public class ExactStringItem extends WordItem {

    public ExactStringItem(String substring) {
        this(substring, false);
    }

    public ExactStringItem(String substring, boolean isFromQuery) {
        super(substring, isFromQuery);
    }

    public ItemType getItemType() {
        return ItemType.EXACT;
    }

    public String getName() {
        return "EXACTSTRING";
    }

    public String stringValue() {
        return getWord();
    }
}
