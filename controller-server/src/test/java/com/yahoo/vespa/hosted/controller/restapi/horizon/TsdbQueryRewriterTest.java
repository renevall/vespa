package com.yahoo.vespa.hosted.controller.restapi.horizon;

import com.yahoo.config.provision.SystemName;
import com.yahoo.config.provision.TenantName;
import com.yahoo.slime.JsonFormat;
import com.yahoo.slime.SlimeUtils;
import com.yahoo.vespa.hosted.controller.api.role.Role;
import org.junit.Test;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.Set;

import static org.junit.Assert.assertEquals;

/**
 * @author valerijf
 */
public class TsdbQueryRewriterTest {

    @Test
    public void rewrites_query() throws IOException {
        assertRewrite("filters-complex.json", "filters-complex.expected.json", Set.of(TenantName.from("tenant2")), false);

        assertRewrite("filter-in-execution-graph.json",
                "filter-in-execution-graph.expected.json",
                Set.of(TenantName.from("tenant2"), TenantName.from("tenant3")), false);

        assertRewrite("filter-in-execution-graph.json",
                "filter-in-execution-graph.expected.operator.json",
                Set.of(TenantName.from("tenant2"), TenantName.from("tenant3")), true);

        assertRewrite("no-filters.json",
                "no-filters.expected.json",
                Set.of(TenantName.from("tenant2"), TenantName.from("tenant3")), false);

        assertRewrite("filters-meta-query.json",
                "filters-meta-query.expected.json",
                Set.of(TenantName.from("tenant2"), TenantName.from("tenant3")), false);
    }

    private static void assertRewrite(String initialFilename, String expectedFilename, Set<TenantName> tenants, boolean operator) throws IOException {
        byte[] data = Files.readAllBytes(Paths.get("src/test/resources/horizon", initialFilename));
        data = TsdbQueryRewriter.rewrite(data, tenants, operator, SystemName.Public);

        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        new JsonFormat(false).encode(baos, SlimeUtils.jsonToSlime(data));
        String expectedJson = Files.readString(Paths.get("src/test/resources/horizon", expectedFilename));

        assertEquals(expectedJson, baos.toString());
    }
}