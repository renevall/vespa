// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include "asyncinitializationpolicy.h"
#include <vespa/config-slobroks.h>
#include <vespa/vdslib/distribution/distribution.h>
#include <vespa/slobrok/imirrorapi.h>
#include <vespa/documentapi/common.h>
#include <vespa/config/subscription/sourcespec.h>

class FRT_Supervisor;
class FNET_Transport;
class FastOS_ThreadPool;

namespace documentapi {

/**
   Super class for routing policies that allow the user to specify external slobrok lists,
   either by supplying external config servers or the slobrok list directly.
*/
class ExternSlobrokPolicy : public AsyncInitializationPolicy
{
protected:
    bool   _firstTry;
    config::ServerSpec::HostSpecList          _configSources;
    std::mutex                                _lock;
    std::unique_ptr<FastOS_ThreadPool>        _threadPool;
    std::unique_ptr<FNET_Transport>           _transport;
    std::unique_ptr<FRT_Supervisor>           _orb;
    std::unique_ptr<slobrok::api::IMirrorAPI> _mirror;
    std::vector<std::string>                  _slobroks;
    string                                    _slobrokConfigId;

public:
    ExternSlobrokPolicy(const std::map<string, string>& params);
    ~ExternSlobrokPolicy() override;

    /**
     * @return a pointer to the slobrok mirror owned by this policy, if any.
     * If the policy uses the default mirror API, NULL is returned.
     */
    const slobrok::api::IMirrorAPI* getMirror() const { return _mirror.get(); }
    slobrok::api::IMirrorAPI::SpecList lookup(mbus::RoutingContext &context, const string& pattern);
    string init() override;
};

}

