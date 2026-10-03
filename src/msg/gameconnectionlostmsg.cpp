#include "msg/gameconnectionlostmsg.h"

#include <iostream>

// NTSC-U/C: 0x003e1a90, PAL: 0x00419f18
GameConnectionLostMsg::GameConnectionLostMsg(const HxStr &reason) : mReason(reason) {
}

// NTSC-U/C: 0x003d7a98, PAL: 0x0040f9a0
Message *GameConnectionLostMsg::New() {
    return new GameConnectionLostMsg;
}

// NTSC-U/C: 0x003e19b0, PAL: 0x00419e30
Message *GameConnectionLostMsg::Clone() {
    return new GameConnectionLostMsg(*this);
}

// NTSC-U/C: 0x003e1a50, PAL: 0x00419ed0
int GameConnectionLostMsg::Type() {
    return g_nGameConnectionLostMsgType;
}

// NTSC-U/C: 0x003e1a60, PAL: 0x00419ee0
const char *GameConnectionLostMsg::Name() {
    return "GameConnectionLostMsg";
}

// NTSC-U/C: 0x003e4070, PAL: 0x0041c2a0
void GameConnectionLostMsg::Print(std::ostream &stream) {
    stream << mReason;
}
