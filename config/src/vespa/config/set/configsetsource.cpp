// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "configsetsource.h"
#include <vespa/config/print/asciiconfigwriter.h>
#include <vespa/config/common/exceptions.h>

#include <vespa/log/log.h>
LOG_SETUP(".config.set.configsetsource");

namespace config {

ConfigSetSource::ConfigSetSource(const IConfigHolder::SP & holder, const ConfigKey & key, const BuilderMapSP & builderMap)
    : _holder(holder),
      _key(key),
      _generation(1),
      _builderMap(builderMap)
{
    if (!validRequest(key))
        throw ConfigRuntimeException("Invalid subscribe for key " + key.toString() + ", not builder found");
}

ConfigSetSource::~ConfigSetSource() { }

void
ConfigSetSource::getConfig()
{
    BuilderMap::const_iterator it(_builderMap->find(_key));
    ConfigInstance * instance = it->second;
    vespalib::asciistream ss;
    AsciiConfigWriter writer(ss);
    writer.write(*instance);
    std::vector<vespalib::string> lines(ss.getlines());
    std::string currentXxhash64(calculateContentXxhash64(lines));

    if (isGenerationNewer(_generation, _lastState.generation) && currentXxhash64.compare(_lastState.xxhash64) != 0) {
        LOG(debug, "New generation, updating");
        _holder->handle(ConfigUpdate::UP(new ConfigUpdate(ConfigValue(lines, currentXxhash64), true, _generation)));
        _lastState.xxhash64 = currentXxhash64;
        _lastState.generation = _generation;
    } else {
        LOG(debug, "Sending timestamp update");
        _holder->handle(ConfigUpdate::UP(new ConfigUpdate(ConfigValue(lines, currentXxhash64), false, _generation)));
        _lastState.generation = _generation;
    }
}

void
ConfigSetSource::reload(int64_t generation)
{
    LOG(debug, "Running update with generation(%" PRId64 ")", generation);
    _generation = generation;
}

void
ConfigSetSource::close()
{
}

bool
ConfigSetSource::validRequest(const ConfigKey & key)
{
    if (_builderMap->find(key) == _builderMap->end())
        return false;
    BuilderMap::const_iterator it(_builderMap->find(key));
    ConfigInstance * instance = it->second;
    return (key.getDefName().compare(instance->defName()) == 0 &&
            key.getDefNamespace().compare(instance->defNamespace()) == 0);
}

} // namespace config
