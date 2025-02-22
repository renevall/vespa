// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.config.subscription;

import org.junit.Test;

import java.io.File;
import java.io.IOException;

import static org.hamcrest.core.Is.is;
import static org.junit.Assert.assertThat;
import static org.junit.Assert.assertTrue;

/**
 * @author Ulf Lilleengen
 */
public class ConfigURITest {

    @Test
    public void testDefaultUri() {
        ConfigURI uri = ConfigURI.createFromId("foo");
        assertThat(uri.getConfigId(), is("foo"));
        assertTrue(uri.getSource() instanceof ConfigSourceSet);
    }

    @Test
    public void testFileUri() throws IOException {
        File file = File.createTempFile("foo", ".cfg");
        ConfigURI uri = ConfigURI.createFromId("file:" + file.getAbsolutePath());
        assertThat(uri.getConfigId(), is(""));
        assertTrue(uri.getSource() instanceof FileSource);
    }

    @Test
    public void testDirUri() throws IOException {
        ConfigURI uri = ConfigURI.createFromId("dir:.");
        assertThat(uri.getConfigId(), is(""));
        assertTrue(uri.getSource() instanceof DirSource);
    }

    @Test
    public void testCustomUri() {
        ConfigURI uri = ConfigURI.createFromIdAndSource("foo", new ConfigSet());
        assertThat(uri.getConfigId(), is("foo"));
        assertTrue(uri.getSource() instanceof ConfigSet);
    }
}
