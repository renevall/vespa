// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "ddbstate.h"
#include <cassert>

namespace proton {

std::vector<vespalib::string> DDBState::_stateNames =
{
    "CONSTRUCT",
    "LOAD",
    "REPLAY_TRANSACTION_LOG",
    "REDO_REPROCESS",
    "APPLY_LIVE_CONFIG",
    "REPROCESS",
    "ONLINE",
    "SHUTDOWN",
    "DEAD",
};

std::vector<vespalib::string> DDBState::_configStateNames =
{
    "OK",
    "NEED_RESTART"
};

DDBState::DDBState()
    : _state(State::CONSTRUCT),
      _configState(ConfigState::OK),
      _lock(),
      _cond()
{
}


DDBState::~DDBState() = default;


bool
DDBState::enterLoadState()
{
    Guard guard(_lock);
    if (getClosed()) {
        return false;
    }
    assert(_state == State::CONSTRUCT);
    _state = State::LOAD;
    return true;
}

    
bool
DDBState::enterReplayTransactionLogState()
{
    Guard guard(_lock);
    if (getClosed()) {
        return false;
    }
    assert(_state == State::LOAD);
    _state = State::REPLAY_TRANSACTION_LOG;
    return true;
}


bool
DDBState::enterRedoReprocessState()
{
    Guard guard(_lock);
    if (getClosed()) {
        return false;
    }
    assert(_state == State::REPLAY_TRANSACTION_LOG);
    _state = State::REDO_REPROCESS;
    return true;
}


bool
DDBState::enterApplyLiveConfigState()
{
    Guard guard(_lock);
    if (getClosed()) {
        return false;
    }
    assert(_state == State::REPLAY_TRANSACTION_LOG ||
           _state == State::REDO_REPROCESS);
    _state = State::APPLY_LIVE_CONFIG;
    return true;
}


bool
DDBState::enterReprocessState()
{
    Guard guard(_lock);
    if (getClosed()) {
        return false;
    }
    assert(_state == State::APPLY_LIVE_CONFIG);
    _state = State::REPROCESS;
    return true;
}

bool
DDBState::enterOnlineState()
{
    Guard guard(_lock);
    if (getClosed()) {
        return false;
    }
    assert(_state == State::REPROCESS);
    _state = State::ONLINE;
    _cond.notify_all();
    return true;
}


void
DDBState::enterShutdownState()
{
    Guard guard(_lock);
    // Shutdown can be initiated before online state was reached
    if (getClosed()) {
        return;
    }
    _state = State::SHUTDOWN;
    _cond.notify_all();
}

void
DDBState::enterDeadState()
{
    Guard guard(_lock);
    if (_state == State::DEAD) {
        return;
    }
    assert(_state == State::SHUTDOWN);
    _state = State::DEAD;
    _cond.notify_all();
}


void
DDBState::setConfigState(ConfigState newConfigState)
{
    Guard guard(_lock);
    _configState = newConfigState;
}


void
DDBState::clearDelayedConfig()
{
    setConfigState(ConfigState::OK);
}


vespalib::string
DDBState::getStateString(State state)
{
    return _stateNames[static_cast<unsigned int>(state)];
}


vespalib::string
DDBState::getConfigStateString(ConfigState configState)
{
    return _configStateNames[static_cast<unsigned int>(configState)];
}


void
DDBState::waitForOnlineState()
{
    GuardLock lk(_lock);
    _cond.wait(lk, [this] { return this->_state >= State::ONLINE; } );
}


} // namespace proton
