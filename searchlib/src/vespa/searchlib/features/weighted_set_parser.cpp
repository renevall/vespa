// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "weighted_set_parser.h"

#include <vespa/log/log.h>
LOG_SETUP(".features.weighted_set_parser");

namespace search::features {

void
WeightedSetParser::logWarning(const vespalib::string &msg)
{
    LOG(warning, "%s", msg.c_str());
}

}
