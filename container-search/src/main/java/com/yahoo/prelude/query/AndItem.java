// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.prelude.query;


/**
 * An and'ing of a collection of sub-expressions
 *
 * @author bratseth
 */
public class AndItem extends CompositeItem {

    public ItemType getItemType() {
        return ItemType.AND;
    }

    public String getName() {
        return "AND";
    }

}
