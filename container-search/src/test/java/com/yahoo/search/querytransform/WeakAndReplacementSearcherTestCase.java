// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.search.querytransform;

import com.yahoo.component.chain.Chain;
import com.yahoo.prelude.query.*;
import com.yahoo.processing.request.CompoundName;
import com.yahoo.search.Query;
import com.yahoo.search.Result;
import com.yahoo.search.Searcher;
import com.yahoo.search.searchchain.Execution;
import org.junit.Test;

import java.util.stream.IntStream;

import static org.junit.Assert.*;

public class WeakAndReplacementSearcherTestCase {

    private static final CompoundName WEAKAND_REPLACE = new CompoundName("weakAnd.replace");
    private static final int N = 99;


    private Execution buildExec() {
        return new Execution(new Chain<Searcher>(new WeakAndReplacementSearcher()),
                Execution.Context.createContextStub());
    }

    private Query buildDefaultQuery(boolean searcherEnabled) {
        Query query = new Query();
        query.properties().set("wand.hits", N);
        query.properties().set(WEAKAND_REPLACE, searcherEnabled);
        OrItem root = new OrItem();
        root.addItem(new WordItem("text"));
        NotItem notItem = new NotItem();
        OrItem notItemOr = new OrItem();
        notItemOr.addItem(new IntItem(1, "index"));
        notItemOr.addItem(new WordItem("positive"));
        notItem.addPositiveItem(notItemOr);
        notItem.addNegativeItem(new WordItem("negative"));
        query.getModel().getQueryTree().setRoot(root);
        return query;
    }




    @Test
    public void requireOrItemsToBeReplaced() {
        Query query = buildDefaultQuery(true);
        Result result = buildExec().search(query);
        Item root = TestUtils.getQueryTreeRoot(result);
        assertFalse(orItemsExist(root));
        assertTrue(root instanceof WeakAndItem);
        assertEquals(N, ((WeakAndItem)root).getN());
    }

    @Test
    public void requireQueryPropertyToWork() {
        Query query = buildDefaultQuery(false);
        Item preRoot = query.getModel().getQueryTree().getRoot();
        Result result = buildExec().search(query);
        Item root = TestUtils.getQueryTreeRoot(result);
        assertTrue(orItemsExist(root));
        assertTrue(deepEquals(root, preRoot));
    }

    @Test
    public void requireDoNothingOnNoOrItems() {
        Query query = new Query();
        query.properties().set(WEAKAND_REPLACE, true);
        AndItem andItem = new AndItem();
        andItem.addItem(new WordItem("1"));
        andItem.addItem(new WordItem("2"));
        query.getModel().getQueryTree().setRoot(andItem);
        Result result = buildExec().search(query);
        Item root = TestUtils.getQueryTreeRoot(result);
        assertTrue(deepEquals(root, andItem));
    }

    @Test
    public void requireChildrenAreTheSame() {
        Query query = new Query();
        query.properties().set(WEAKAND_REPLACE, true);
        OrItem preRoot = new OrItem();
        preRoot.addItem(new WordItem("val1"));
        preRoot.addItem(new WordItem("val2"));

        query.getModel().getQueryTree().setRoot(preRoot);
        Result result = buildExec().search(query);
        WeakAndItem root = (WeakAndItem)TestUtils.getQueryTreeRoot(result);
        assertEquals(preRoot.getItem(0), root.getItem(0));
        assertEquals(preRoot.getItem(1), root.getItem(1));
    }

    private boolean deepEquals(Item item1, Item item2) {
        if (item1 != item2) {
            return false;
        }
        if (!(item1 instanceof CompositeItem)) {
            return true;
        }

        CompositeItem compositeItem1 = (CompositeItem) item1;
        CompositeItem compositeItem2 = (CompositeItem) item2;
        return IntStream.range(0, compositeItem1.getItemCount())
                .allMatch(i -> deepEquals(compositeItem1.getItem(i), compositeItem2.getItem(i)));
    }

    private boolean orItemsExist(Item item) {
        if (!(item instanceof CompositeItem)) {
            return false;
        }
        if (item instanceof OrItem) {
            return true;
        }
        CompositeItem compositeItem = (CompositeItem) item;
        return compositeItem.items().stream().anyMatch(this::orItemsExist);
    }

}
