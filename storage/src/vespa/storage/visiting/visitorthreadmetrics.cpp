// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "visitorthreadmetrics.h"

namespace storage {

// TODO Vespa 8 all metrics with .sum in the name should have that removed.
VisitorThreadMetrics::VisitorThreadMetrics(const std::string& name, const std::string& desc)
    : metrics::MetricSet(name, {{"visitor"},{"partofsum"},{"thread"}}, desc),
      queueSize("queuesize", {}, "Size of input message queue.", this),
      averageQueueWaitingTime("averagequeuewait.sum", {}, "Average time an operation spends in input queue.", this),
      averageVisitorLifeTime("averagevisitorlifetime.sum", {}, "Average lifetime of a visitor", this),
      averageVisitorCreationTime("averagevisitorcreationtime", {}, "Average time spent creating a visitor instance", this),
      averageMessageSendTime("averagemessagesendtime.sum", {}, "Average time it takes for messages to be sent to their target (and be replied to)", this),
      averageProcessingTime("averageprocessingtime.sum", {}, "Average time visitor uses in handleDocuments() call", this),
      createdVisitors("created.sum", {}, "Number of visitors created.", this),
      abortedVisitors("aborted", {}, "Number of visitors aborted.", this),
      completedVisitors("completed.sum", {}, "Number of visitors completed", this),
      failedVisitors("failed.sum", {}, "Number of visitors failed", this),
      visitorDestinationFailureReplies("destination_failure_replies", {},"Number of failure replies received from the visitor destination", this)
{
    queueSize.unsetOnZeroValue();
}

VisitorThreadMetrics::~VisitorThreadMetrics() = default;

}
