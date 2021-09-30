// Copyright 2017 Yahoo Holdings. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include "operationstarter.h"
#include <vespa/storage/distributor/maintenance/pending_window_checker.h>
#include <vespa/vespalib/util/hdr_abort.h>
#include <vespa/storage/distributor/operations/operation.h>

namespace storage::distributor {

class ThrottlingOperationStarter : public OperationStarter, public PendingWindowChecker
{
    class ThrottlingOperation : public Operation
    {
    public:
        ThrottlingOperation(const Operation::SP& operation,
                            ThrottlingOperationStarter& operationStarter)
            : _operation(operation),
              _operationStarter(operationStarter)
        {}

        ~ThrottlingOperation() override;
    private:
        Operation::SP _operation;
        ThrottlingOperationStarter& _operationStarter;

        ThrottlingOperation(const ThrottlingOperation&);
        ThrottlingOperation& operator=(const ThrottlingOperation&);
        
        void onClose(DistributorStripeMessageSender& sender) override {
            _operation->onClose(sender);
        }
        const char* getName() const override {
            return _operation->getName();
        }
        std::string getStatus() const override {
            return _operation->getStatus();
        }
        std::string toString() const override {
            return _operation->toString();
        }
        void start(DistributorStripeMessageSender& sender, framework::MilliSecTime startTime) override {
            _operation->start(sender, startTime);
        }
        void receive(DistributorStripeMessageSender& sender, const std::shared_ptr<api::StorageReply> & msg) override {
            _operation->receive(sender, msg);
        }
        framework::MilliSecTime getStartTime() const {
            return _operation->getStartTime();
        }
        void onStart(DistributorStripeMessageSender&) override {
            // Should never be called directly on the throttled operation
            // instance, but rather on its wrapped implementation.
            HDR_ABORT("should not be reached");
        }
        void onReceive(DistributorStripeMessageSender&,
                       const std::shared_ptr<api::StorageReply>&) override {
            HDR_ABORT("should not be reached");
        }
    };

    OperationStarter& _starterImpl;
public:
    ThrottlingOperationStarter(OperationStarter& starterImpl)
        : _starterImpl(starterImpl),
          _minPending(0),
          _maxPending(UINT32_MAX),
          _pendingCount(0)
    {}
    ~ThrottlingOperationStarter() override;

    bool start(const std::shared_ptr<Operation>& operation, Priority priority) override;

    bool may_allow_operation_with_priority(Priority priority) const noexcept override;

    bool canStart(uint32_t currentOperationCount, Priority priority) const;

    void setMaxPendingRange(uint32_t minPending, uint32_t maxPending) {
        _minPending = minPending;
        _maxPending = maxPending;
    }

private:
    ThrottlingOperationStarter(const ThrottlingOperationStarter&);
    ThrottlingOperationStarter& operator=(const ThrottlingOperationStarter&);

    friend class ThrottlingOperation;
    void signalOperationFinished(const Operation& op);

    uint32_t _minPending;
    uint32_t _maxPending;
    uint32_t _pendingCount;
};

}
