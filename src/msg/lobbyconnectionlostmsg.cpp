#include "msg/lobbyconnectionlostmsg.h"

#include <iostream>

// 0x003e1d10
LobbyConnectionLostMsg::LobbyConnectionLostMsg(const HxStr &unknown04) : mUnknown04(unknown04) {
}

// 0x003d7ad8
Message *LobbyConnectionLostMsg::New() {
    return new LobbyConnectionLostMsg;
}

// 0x003e1c30
Message *LobbyConnectionLostMsg::Clone() {
    return new LobbyConnectionLostMsg(*this);
}

// 0x003e1cd0
int LobbyConnectionLostMsg::Type() {
    return g_nLobbyConnectionLostMsgType;
}

// 0x003e1ce0
const char *LobbyConnectionLostMsg::Name() {
    return "LobbyConnectionLostMsg";
}

// 0x003e4098
void LobbyConnectionLostMsg::Print(std::ostream &) {
}
