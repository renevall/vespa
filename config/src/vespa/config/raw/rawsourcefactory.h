// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <vespa/config/common/sourcefactory.h>

namespace config {

/**
 * Factory for RawSource
 */
class RawSourceFactory : public SourceFactory {
public:
    RawSourceFactory(const vespalib::string & payload)
        : _payload(payload)
    { }

    Source::UP createSource(const IConfigHolder::SP & holder, const ConfigKey & key) const override;
private:
    const vespalib::string _payload;
};

}

