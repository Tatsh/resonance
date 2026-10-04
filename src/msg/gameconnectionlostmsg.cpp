#include "msg/gameconnectionlostmsg.h"

#include <iostream>

GameConnectionLostMsg::GameConnectionLostMsg(const HxStr &reason) : mReason(reason) {
}

Message *GameConnectionLostMsg::New() {
    return new GameConnectionLostMsg;
}

Message *GameConnectionLostMsg::Clone() {
    return new GameConnectionLostMsg(*this);
}

int GameConnectionLostMsg::Type() {
    return g_nGameConnectionLostMsgType;
}

const char *GameConnectionLostMsg::GetName() const {
    return "GameConnectionLostMsg";
}

void GameConnectionLostMsg::PrintExtra(std::ostream &stream) const {
    stream << mReason;
}
