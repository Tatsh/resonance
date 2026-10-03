#include "msg/lobbyconnectionlostmsg.h"

#include <iostream>

// NTSC-U/C: 0x003e1d10, PAL: 0x0041a1b0
LobbyConnectionLostMsg::LobbyConnectionLostMsg(const HxStr &reason) : mReason(reason) {
}

// NTSC-U/C: 0x003d7ad8, PAL: 0x0040f9e8
Message *LobbyConnectionLostMsg::New() {
    return new LobbyConnectionLostMsg;
}

// NTSC-U/C: 0x003e1c30, PAL: 0x0041a0c8
Message *LobbyConnectionLostMsg::Clone() {
    return new LobbyConnectionLostMsg(*this);
}

// NTSC-U/C: 0x003e1cd0, PAL: 0x0041a168
int LobbyConnectionLostMsg::Type() {
    return g_nLobbyConnectionLostMsgType;
}

// NTSC-U/C: 0x003e1ce0, PAL: 0x0041a178
const char *LobbyConnectionLostMsg::Name() {
    return "LobbyConnectionLostMsg";
}

// NTSC-U/C: 0x003e4098, PAL: 0x0041c2c8
void LobbyConnectionLostMsg::Print(std::ostream &) {
}
