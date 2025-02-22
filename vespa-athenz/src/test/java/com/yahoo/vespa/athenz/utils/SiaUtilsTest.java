// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.athenz.utils;

import com.yahoo.vespa.athenz.api.AthenzIdentity;
import com.yahoo.vespa.athenz.api.AthenzService;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

/**
 * @author bjorncs
 */
public class SiaUtilsTest {

    @Rule
    public TemporaryFolder tempDirectory = new TemporaryFolder();

    @Test
    public void it_finds_all_identity_names_from_files_in_sia_keys_directory() throws IOException {
        Path siaRoot = tempDirectory.getRoot().toPath();
        assertTrue(SiaUtils.findSiaServices(siaRoot).isEmpty());
        Files.createDirectory(siaRoot.resolve("keys"));
        AthenzService fooService = new AthenzService("my.domain.foo");
        Files.createFile(SiaUtils.getPrivateKeyFile(siaRoot, fooService));
        AthenzService barService = new AthenzService("my.domain.bar");
        Files.createFile(SiaUtils.getPrivateKeyFile(siaRoot, barService));

        List<AthenzIdentity> siaIdentities = SiaUtils.findSiaServices(siaRoot);
        assertEquals(2, siaIdentities.size());
        assertTrue(siaIdentities.contains(fooService));
        assertTrue(siaIdentities.contains(barService));
    }

}
