# Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
vespa_add_library(storageframework INTERFACE
    SOURCES
    INSTALL lib64
    DEPENDS
    storageframework_defaultimplementation
    storageframework_generic
)
