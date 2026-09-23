#include "msg/gameconnectionlostmsg.h"

#include <iostream>

// 0x003e1a90
GameConnectionLostMsg::GameConnectionLostMsg(const HxStr &unknown04) : mUnknown04(unknown04) {
}

// 0x003d7a98
Message *GameConnectionLostMsg::New() {
    return new GameConnectionLostMsg;
}

// 0x003e19b0
Message *GameConnectionLostMsg::Clone() {
    return new GameConnectionLostMsg(*this);
}

// 0x003e1a50
int GameConnectionLostMsg::Type() {
    return g_nGameConnectionLostMsgType;
}

// 0x003e1a60
const char *GameConnectionLostMsg::Name() {
    return "GameConnectionLostMsg";
}

// 0x003e4070
void GameConnectionLostMsg::Print(std::ostream &stream) {
    stream << mUnknown04;
}
