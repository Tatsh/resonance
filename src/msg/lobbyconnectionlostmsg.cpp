#include "msg/lobbyconnectionlostmsg.h"

#include <iostream>

LobbyConnectionLostMsg::LobbyConnectionLostMsg(const HxStr &reason) : mReason(reason) {
}

Message *LobbyConnectionLostMsg::New() {
    return new LobbyConnectionLostMsg;
}

Message *LobbyConnectionLostMsg::Clone() {
    return new LobbyConnectionLostMsg(*this);
}

int LobbyConnectionLostMsg::Type() {
    return g_nLobbyConnectionLostMsgType;
}

const char *LobbyConnectionLostMsg::GetName() const {
    return "LobbyConnectionLostMsg";
}

void LobbyConnectionLostMsg::PrintExtra(std::ostream &) const {
}
