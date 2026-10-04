#include "msg/gameconnectfailuremsg.h"

#include <iostream>

GameConnectFailureMsg::GameConnectFailureMsg(const HxStr &reason) : mReason(reason) {
}

Message *GameConnectFailureMsg::New() {
    return new GameConnectFailureMsg;
}

Message *GameConnectFailureMsg::Clone() {
    return new GameConnectFailureMsg(*this);
}

int GameConnectFailureMsg::Type() {
    return g_nGameConnectFailureMsgType;
}

const char *GameConnectFailureMsg::GetName() const {
    return "GameConnectFailureMsg";
}

void GameConnectFailureMsg::PrintExtra(std::ostream &stream) const {
    stream << mReason;
}
