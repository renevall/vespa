// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.prelude.query;


/**
 * A word that matches substrings of words
 *
 * @author banino
 */
public class SubstringItem extends WordItem {

    public SubstringItem(String substring) {
        this(substring, false);
    }

    public SubstringItem(String substring, boolean isFromQuery) {
        super(substring, isFromQuery);
    }

    public ItemType getItemType() {
        return ItemType.SUBSTRING;
    }

    public String getName() {
        return "SUBSTRING";
    }

    public String stringValue() {
        return "*" + getWord() + "*";
    }

}
