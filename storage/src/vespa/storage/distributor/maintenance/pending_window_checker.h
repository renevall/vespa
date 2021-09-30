// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <vespa/storage/distributor/operationstarter.h>

namespace storage::distributor {

class PendingWindowChecker {
public:
    virtual ~PendingWindowChecker() = default;
    [[nodiscard]] virtual bool may_allow_operation_with_priority(OperationStarter::Priority) const noexcept = 0;
};

}
