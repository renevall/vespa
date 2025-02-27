// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.searchdefinition.processing;

import com.yahoo.searchdefinition.parser.ParseException;
import org.junit.Test;

import java.io.IOException;

import static com.yahoo.searchdefinition.processing.AssertSearchBuilder.assertBuildFails;
import static com.yahoo.searchdefinition.processing.AssertSearchBuilder.assertBuilds;

/**
 * @author Simon Thoresen Hult
 */
public class IndexingValuesTestCase {

    @Test
    public void requireThatModifyFieldNoOutputDoesNotThrow() throws IOException, ParseException {
        assertBuilds("src/test/examples/indexing_modify_field_no_output.sd");
    }

    @Test
    public void requireThatInputOtherFieldThrows() throws IOException, ParseException {
        assertBuildFails("src/test/examples/indexing_input_other_field.sd",
                         "For search 'indexing_input_other_field', field 'bar': Indexing expression 'input foo' " +
                         "attempts to modify the value of the document field 'bar'. " +
                         "Use a field outside the document block instead.");
    }

}
