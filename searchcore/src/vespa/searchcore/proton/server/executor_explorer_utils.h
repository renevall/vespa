// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

namespace vespalib { class ThreadExecutor; }
namespace vespalib::slime { struct Cursor; }

namespace proton::explorer {

/**
 * Utility to convert an executor to slime for use with a state explorer.
 */
void convert_executor_to_slime(const vespalib::ThreadExecutor* executor, vespalib::slime::Cursor& object);

}

