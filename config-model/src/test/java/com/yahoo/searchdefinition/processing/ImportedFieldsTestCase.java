// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.searchdefinition.processing;

import com.yahoo.searchdefinition.Search;
import com.yahoo.searchdefinition.SearchBuilder;
import com.yahoo.searchdefinition.derived.AttributeFields;
import com.yahoo.searchdefinition.document.ImportedComplexField;
import com.yahoo.searchdefinition.document.ImportedField;
import com.yahoo.searchdefinition.parser.ParseException;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ExpectedException;

import static org.junit.Assert.assertEquals;
import static com.yahoo.config.model.test.TestUtil.joinLines;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

/**
 * @author geirst
 */
public class ImportedFieldsTestCase {

    @Test
    public void fields_can_be_imported_from_referenced_document_types() throws ParseException {
        Search search = buildAdSearch(joinLines(
                "search ad {",
                "  document ad {",
                "    field campaign_ref type reference<campaign> { indexing: attribute }",
                "    field person_ref type reference<person> { indexing: attribute }",
                "  }",
                "  import field campaign_ref.budget as my_budget {}",
                "  import field person_ref.name as my_name {}",
                "}"));
        assertEquals(2, search.importedFields().get().fields().size());
        assertSearchContainsImportedField("my_budget", "campaign_ref", "campaign", "budget", search);
        assertSearchContainsImportedField("my_name", "person_ref", "person", "name", search);
    }

    @Rule
    public ExpectedException exception = ExpectedException.none();

    @Test
    public void field_reference_spec_must_include_dot() throws ParseException {
        exception.expect(IllegalArgumentException.class);
        exception.expectMessage("Illegal field reference spec 'campaignrefbudget': Does not include a single '.'");
        buildAdSearch(joinLines(
                "search ad {",
                "  document ad {}",
                "  import field campaignrefbudget as budget {}",
                "}"));
    }

    @Test
    public void fail_duplicate_import() throws ParseException {
        exception.expect(IllegalArgumentException.class);
        exception.expectMessage("For search 'ad', import field as 'my_budget': Field already imported");
        Search search = buildAdSearch(joinLines(
                "search ad {",
                "  document ad {",
                "    field campaign_ref type reference<campaign> { indexing: attribute }",
                "  }",
                "  import field campaign_ref.budget as my_budget {}",
                "  import field campaign_ref.budget as my_budget {}",
                "}"));
    }

    private static Search buildAdSearch(String sdContent) throws ParseException {
        SearchBuilder builder = new SearchBuilder();
        builder.importString(joinLines("search campaign {",
                "  document campaign {",
                "    field budget type int { indexing: attribute }",
                "  }",
                "}"));
        builder.importString(joinLines("search person {",
                "  document person {",
                "    field name type string { indexing: attribute }",
                "  }",
                "}"));
        builder.importString(sdContent);
        builder.build();
        return builder.getSearch("ad");
    }

    private static void checkStructImport(AncestorStructSdBuilder parentBuilder) throws ParseException {
        Search search = buildChildSearch(parentBuilder.build(), new ChildStructSdBuilder().build());
        checkImportedStructFields(search, parentBuilder);
    }

    private static void checkNestedStructImport(AncestorStructSdBuilder grandParentBuilder) throws ParseException {
        Search search = buildChildSearch(grandParentBuilder.build(),
                new IntermediateParentStructSdBuilder().build(),
                new ChildStructSdBuilder().build());
        checkImportedStructFields(search, grandParentBuilder);
    }

    private static void checkImportedStructFields(Search search, AncestorStructSdBuilder ancestorBuilder) {
        assertEquals(3, search.importedFields().get().fields().size());
        checkImportedField("my_elem_array.name", "parent_ref", "parent", "elem_array.name", search, ancestorBuilder.elem_array_name_attr);
        checkImportedField("my_elem_array.weight", "parent_ref", "parent", "elem_array.weight", search, ancestorBuilder.elem_array_weight_attr);
        checkImportedField("my_elem_map.key", "parent_ref", "parent", "elem_map.key", search, ancestorBuilder.elem_map_key_attr);
        checkImportedField("my_elem_map.value.name", "parent_ref", "parent", "elem_map.value.name", search, ancestorBuilder.elem_map_value_name_attr);
        checkImportedField("my_elem_map.value.weight", "parent_ref", "parent", "elem_map.value.weight", search, ancestorBuilder.elem_map_value_weight_attr);
        checkImportedField("my_str_int_map.key", "parent_ref", "parent", "str_int_map.key", search, ancestorBuilder.str_int_map_key_attr);
        checkImportedField("my_str_int_map.value", "parent_ref", "parent", "str_int_map.value", search, ancestorBuilder.str_int_map_value_attr);
        checkImportedField("my_elem_array", "parent_ref", "parent", "elem_array", search, true);
        checkImportedField("my_elem_map", "parent_ref", "parent", "elem_map", search, true);
        checkImportedField("my_str_int_map", "parent_ref", "parent", "str_int_map", search, true);
    }

    @Test
    public void check_struct_import() throws ParseException {
        checkStructImport(new ParentStructSdBuilder());
        checkStructImport(new ParentStructSdBuilder().elem_array_weight_attr(false).elem_map_value_weight_attr(false));
        checkStructImport(new ParentStructSdBuilder().elem_array_name_attr(false).elem_map_value_name_attr(false));
    }

    @Test
    public void check_nested_struct_import() throws ParseException {
        checkNestedStructImport(new GrandParentStructSdBuilder());
        checkNestedStructImport(new GrandParentStructSdBuilder().elem_array_weight_attr(false).elem_map_value_weight_attr(false));
        checkNestedStructImport(new GrandParentStructSdBuilder().elem_array_name_attr(false).elem_map_value_name_attr(false));
    }

    @Test
    public void check_illegal_struct_import_missing_array_of_struct_attributes() throws ParseException {
        exception.expect(IllegalArgumentException.class);
        exception.expectMessage("For search 'child', import field 'my_elem_array': Field 'elem_array' via reference field 'parent_ref': Is not a struct containing an attribute field.");
        checkStructImport(new ParentStructSdBuilder().elem_array_name_attr(false).elem_array_weight_attr(false));
    }

    @Test
    public void check_illegal_struct_import_missing_map_of_struct_key_attribute() throws ParseException {
        exception.expect(IllegalArgumentException.class);
        exception.expectMessage("For search 'child', import field 'my_elem_map' (nested to 'my_elem_map.key'): Field 'elem_map.key' via reference field 'parent_ref': Is not an attribute field. Only attribute fields supported");
        checkStructImport(new ParentStructSdBuilder().elem_map_key_attr(false));
    }

    @Test
    public void check_illegal_struct_import_missing_map_of_struct_value_attributes() throws ParseException {
        exception.expect(IllegalArgumentException.class);
        exception.expectMessage("For search 'child', import field 'my_elem_map' (nested to 'my_elem_map.value'): Field 'elem_map.value' via reference field 'parent_ref': Is not a struct containing an attribute field.");
        checkStructImport(new ParentStructSdBuilder().elem_map_value_name_attr(false).elem_map_value_weight_attr(false));
    }

    @Test
    public void check_illegal_struct_import_missing_map_of_primitive_key_attribute() throws ParseException {
        exception.expect(IllegalArgumentException.class);
        exception.expectMessage("For search 'child', import field 'my_str_int_map' (nested to 'my_str_int_map.key'): Field 'str_int_map.key' via reference field 'parent_ref': Is not an attribute field. Only attribute fields supported");
        checkStructImport(new ParentStructSdBuilder().str_int_map_key_attr(false));
    }

    @Test
    public void check_illegal_struct_import_missing_map_of_primitive_value_attribute() throws ParseException {
        exception.expect(IllegalArgumentException.class);
        exception.expectMessage("For search 'child', import field 'my_str_int_map' (nested to 'my_str_int_map.value'): Field 'str_int_map.value' via reference field 'parent_ref': Is not an attribute field. Only attribute fields supported");
        checkStructImport(new ParentStructSdBuilder().str_int_map_value_attr(false));
    }

    private static class NamedSdBuilder {
        protected String name;
        private String fieldPrefix;

        public NamedSdBuilder(String name, String fieldPrefix) {
            this.name = name;
            this.fieldPrefix = fieldPrefix;
        }

        protected String prefixedFieldName(String name) {
            return fieldPrefix + name;
        }
    }

    private static class AncestorStructSdBuilder extends NamedSdBuilder {
        private boolean elem_array_name_attr;
        private boolean elem_array_weight_attr;
        private boolean elem_map_key_attr;
        private boolean elem_map_value_name_attr;
        private boolean elem_map_value_weight_attr;
        private boolean str_int_map_key_attr;
        private boolean str_int_map_value_attr;

        public AncestorStructSdBuilder(String name, String fieldPrefix) {
            super(name, fieldPrefix);
            elem_array_name_attr = true;
            elem_array_weight_attr = true;
            elem_map_key_attr = true;
            elem_map_value_name_attr = true;
            elem_map_value_weight_attr = true;
            str_int_map_key_attr = true;
            str_int_map_value_attr = true;
        }

        public AncestorStructSdBuilder elem_array_name_attr(boolean v) { elem_array_name_attr = v; return this; }
        public AncestorStructSdBuilder elem_array_weight_attr(boolean v) { elem_array_weight_attr = v; return this; }
        public AncestorStructSdBuilder elem_map_key_attr(boolean v) { elem_map_key_attr = v; return this; }
        public AncestorStructSdBuilder elem_map_value_name_attr(boolean v) { elem_map_value_name_attr = v; return this; }
        public AncestorStructSdBuilder elem_map_value_weight_attr(boolean v) { elem_map_value_weight_attr = v; return this; }
        public AncestorStructSdBuilder str_int_map_key_attr(boolean v) { str_int_map_key_attr = v; return this; }
        public AncestorStructSdBuilder str_int_map_value_attr(boolean v) { str_int_map_value_attr = v; return this; }

        public String build() {
            return joinLines("search " + name + " {",
                    "  document " + name + " {",
                    "    struct elem {",
                    "      field name type string {}",
                    "      field weight type int {}",
                    "    }",
                    "    field " + prefixedFieldName("elem_array") + " type array<elem> {",
                    "      indexing: summary",
                    "      struct-field name {",
                    structFieldSpec(elem_array_name_attr),
                    "      }",
                    "      struct-field weight {",
                    structFieldSpec(elem_array_weight_attr),
                    "      }",
                    "    }",
                    "    field " + prefixedFieldName("elem_map") + " type map<string, elem> {",
                    "      indexing: summary",
                    "      struct-field key {",
                    structFieldSpec(elem_map_key_attr),
                    "      }",
                    "      struct-field value.name {",
                    structFieldSpec(elem_map_value_name_attr),
                    "      }",
                    "      struct-field value.weight {",
                    structFieldSpec(elem_map_value_weight_attr),
                    "      }",
                    "    }",
                    "    field " + prefixedFieldName("str_int_map") + " type map<string, int> {",
                    "      indexing: summary",
                    "      struct-field key {",
                    structFieldSpec(str_int_map_key_attr),
                    "      }",
                    "      struct-field value {",
                    structFieldSpec(str_int_map_value_attr),
                    "      }",
                    "    }",
                    "  }",
                    "}");
        }

        private static String structFieldSpec(boolean isAttribute) {
            return isAttribute ? "        indexing: attribute" : "";
        }
    }

    private static class ParentStructSdBuilder extends AncestorStructSdBuilder {
        ParentStructSdBuilder() {
            super("parent", "");
        }
    }

    private static class GrandParentStructSdBuilder extends AncestorStructSdBuilder {
        GrandParentStructSdBuilder() {
            super("grandparent", "gp_");
        }
    }

    private static class DescendantSdBuilder extends NamedSdBuilder {
        protected String parentName;
        private String parentFieldPrefix;

        public DescendantSdBuilder(String name, String fieldPrefix, String parentName, String parentFieldPrefix) {
            super(name, fieldPrefix);
            this.parentName = parentName;
            this.parentFieldPrefix = parentFieldPrefix;
        }

        protected String parentRef() {
            return parentName + "_ref";
        }

        protected String importParentField(String fieldName) {
            return "  import field " + parentRef() + "." + parentFieldPrefix + fieldName + " as " + prefixedFieldName(fieldName) + " {}";
        }
    }

    private static class DescendantStructSdBuilder extends DescendantSdBuilder {
        public DescendantStructSdBuilder(String name, String fieldPrefix, String parentName, String parentFieldPrefix) {
            super(name, fieldPrefix, parentName, parentFieldPrefix);
        }

        public String build() {
            return joinLines("search " + name + " {",
                    "  document " + name + " {",
                    "    field " + parentRef() + " type reference<" + parentName + "> {",
                    "      indexing: attribute | summary",
                    "    }",
                    "  }",
                    importParentField("elem_array"),
                    importParentField("elem_map"),
                    importParentField("str_int_map"),
                    "}");
        }
    }

    private static class ChildStructSdBuilder extends DescendantStructSdBuilder {
        public ChildStructSdBuilder() {
            super("child", "my_", "parent", "");
        }
    }

    private static class IntermediateParentStructSdBuilder extends DescendantStructSdBuilder {
        public IntermediateParentStructSdBuilder() {
            super("parent", "", "grandparent", "gp_");
        }
    }

    private static Search buildChildSearch(String parentSdContent, String sdContent) throws ParseException {
        SearchBuilder builder = new SearchBuilder();
        builder.importString(parentSdContent);
        builder.importString(sdContent);
        builder.build();
        return builder.getSearch("child");
    }

    private static Search buildChildSearch(String grandParentSdContent, String parentSdContent, String sdContent) throws ParseException {
        SearchBuilder builder = new SearchBuilder();
        builder.importString(grandParentSdContent);
        builder.importString(parentSdContent);
        builder.importString(sdContent);
        builder.build();
        return builder.getSearch("child");
    }

    private static class AncestorPosSdBuilder extends NamedSdBuilder {
        public AncestorPosSdBuilder(String name, String fieldPrefix) {
            super(name, fieldPrefix);
        }

        public String build() {
            return joinLines("search " + name + " {",
                    "  document " + name + " {",
                    "field " + prefixedFieldName("pos") + " type position {",
                    "indexing: attribute | summary",
                    "    }",
                    "  }",
                    "}");
        }
    }

    private static class ParentPosSdBuilder extends AncestorPosSdBuilder {
        public ParentPosSdBuilder() { super("parent", ""); }
    }

    private static class GrandParentPosSdBuilder extends AncestorPosSdBuilder {
        public GrandParentPosSdBuilder() { super("grandparent", "gp_"); }
    }

    private static class DescendantPosSdBuilder extends DescendantSdBuilder {
        private boolean import_pos_zcurve_before;

        public DescendantPosSdBuilder(String name, String fieldPrefix, String parentName, String parentFieldPrefix) {
            super(name, fieldPrefix, parentName, parentFieldPrefix);
            import_pos_zcurve_before = false;
        }

        DescendantPosSdBuilder import_pos_zcurve_before(boolean v) { import_pos_zcurve_before = v; return this; }

        public String build() {
            return joinLines("search " + name + " {",
                    "  document " + name + " {",
                    "    field " + parentRef() + " type reference<" + parentName + "> {",
                    "      indexing: attribute | summary",
                    "    }",
                    "  }",
                    importPosZCurve(import_pos_zcurve_before),
                    importParentField("pos"),
                    "}");
        }

        private static String importPosZCurve(boolean doImport) {
            return doImport ? "import field parent_ref.pos_zcurve as my_pos_zcurve {}" : "";
        }
    }

    private static class ChildPosSdBuilder extends DescendantPosSdBuilder {
        public ChildPosSdBuilder() {
            super("child", "my_", "parent", "");
        }
    }

    private static class IntermediateParentPosSdBuilder extends DescendantPosSdBuilder {
        public IntermediateParentPosSdBuilder() {
            super("parent", "", "grandparent", "gp_");
        }
    }

    private static void checkPosImport(ParentPosSdBuilder parentBuilder, DescendantPosSdBuilder childBuilder) throws ParseException {
        Search search = buildChildSearch(parentBuilder.build(), childBuilder.build());
        checkImportedPosFields(search);
    }

    private static void checkNestedPosImport(GrandParentPosSdBuilder grandParentBuilder, DescendantPosSdBuilder childBuilder) throws ParseException {
        Search search = buildChildSearch(grandParentBuilder.build(), new IntermediateParentPosSdBuilder().build(), childBuilder.build());
        checkImportedPosFields(search);
    }

    private static void checkImportedPosFields(Search search) {
        assertEquals(2, search.importedFields().get().fields().size());
        assertSearchContainsImportedField("my_pos_zcurve", "parent_ref", "parent", "pos_zcurve", search);
        assertSearchContainsImportedField("my_pos", "parent_ref", "parent", "pos", search);
    }

    @Test
    public void check_pos_import() throws ParseException {
        checkPosImport(new ParentPosSdBuilder(), new ChildPosSdBuilder());
    }

    @Test
    public void check_nested_pos_import() throws ParseException {
        checkNestedPosImport(new GrandParentPosSdBuilder(), new ChildPosSdBuilder());
    }

    @Test
    public void check_pos_import_after_pos_zcurve_import() throws ParseException {
        exception.expect(IllegalArgumentException.class);
        exception.expectMessage("For search 'child', import field 'my_pos_zcurve': Field 'pos_zcurve' via reference field 'parent_ref': Field already imported");
        checkPosImport(new ParentPosSdBuilder(), new ChildPosSdBuilder().import_pos_zcurve_before(true));
    }

    private static ImportedField getImportedField(String name, Search search) {
        if (name.contains(".")) {
            assertNull(search.importedFields().get().fields().get(name));
            String superFieldName = name.substring(0,name.indexOf("."));
            String subFieldName = name.substring(name.indexOf(".")+1);
            ImportedField superField = search.importedFields().get().fields().get(superFieldName);
            if (superField != null && superField instanceof ImportedComplexField) {
                return ((ImportedComplexField)superField).getNestedField(subFieldName);
            }
            return null;
        }
        return search.importedFields().get().fields().get(name);
    }

    private static void assertSearchNotContainsImportedField(String fieldName, Search search) {
        ImportedField importedField = getImportedField(fieldName, search);
        assertNull(importedField);
    }

    private static void assertSearchContainsImportedField(String fieldName,
                                                          String referenceFieldName,
                                                          String referenceDocType,
                                                          String targetFieldName,
                                                          Search search) {
        ImportedField importedField = getImportedField(fieldName, search);
        assertNotNull(importedField);
        assertEquals(fieldName, importedField.fieldName());
        assertEquals(referenceFieldName, importedField.reference().referenceField().getName());
        assertEquals(referenceDocType, importedField.reference().targetSearch().getName());
        assertEquals(targetFieldName, importedField.targetField().getName());
    }

    private static void checkImportedField(String fieldName, String referenceFieldName, String referenceDocType,
                                           String targetFieldName, Search search, boolean present) {
        if (present) {
            assertSearchContainsImportedField(fieldName, referenceFieldName, referenceDocType, targetFieldName, search);
        } else {
            assertSearchNotContainsImportedField(fieldName, search);
        }
    }

    @Test
    public void field_with_struct_field_attributes_can_be_imported_from_parents_that_use_inheritance() throws ParseException {
        var builder = buildParentsUsingInheritance();

        assertParentContainsEntriesAttributes(builder.getSearch("parent_a"));
        assertParentContainsEntriesAttributes(builder.getSearch("parent_b"));

        var child = builder.getSearch("child");
        checkImportedField("entries_from_a", "ref_parent_a", "parent_a", "entries", child, true);
        checkImportedField("entries_from_a.key", "ref_parent_a", "parent_a", "entries.key", child, true);
        checkImportedField("entries_from_a.value", "ref_parent_a", "parent_a", "entries.value", child, true);

        checkImportedField("entries_from_b", "ref_parent_b", "parent_b", "entries", child, true);
        checkImportedField("entries_from_b.key", "ref_parent_b", "parent_b", "entries.key", child, true);
        checkImportedField("entries_from_b.value", "ref_parent_b", "parent_b", "entries.value", child, true);
    }

    private void assertParentContainsEntriesAttributes(Search parent) {
        var attrs = new AttributeFields(parent);
        assertTrue(attrs.containsAttribute("entries.key"));
        assertTrue(attrs.containsAttribute("entries.value"));
    }

    private SearchBuilder buildParentsUsingInheritance() throws ParseException {
        var builder = new SearchBuilder();
        builder.importString(joinLines("schema parent_a {",
                "document parent_a {",
                "  struct Entry {",
                "    field key type string {}",
                "    field value type string {}",
                "  }",
                "  field entries type array<Entry> {",
                "    indexing: summary",
                "    struct-field key { indexing: attribute }",
                "    struct-field value { indexing: attribute }",
                "  }",
                "}",
                "}"));

        builder.importString(joinLines("schema parent_b {",
                "document parent_b inherits parent_a {",
                "}",
                "}"));

        builder.importString(joinLines("schema child {",
                "document child {",
                "  field ref_parent_a type reference<parent_a> {",
                "    indexing: attribute",
                "  }",
                "  field ref_parent_b type reference<parent_b> {",
                "    indexing: attribute",
                "  }",
                "}",
                "import field ref_parent_a.entries as entries_from_a {}",
                "import field ref_parent_b.entries as entries_from_b {}",
                "}"));

        builder.build();
        return builder;
    }

    }
