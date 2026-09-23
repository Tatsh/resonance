#include "msg/gameconnectfailuremsg.h"

#include <iostream>

// 0x003e1810
GameConnectFailureMsg::GameConnectFailureMsg(const HxStr &unknown04) : mUnknown04(unknown04) {
}

// 0x003d7a58
Message *GameConnectFailureMsg::New() {
    return new GameConnectFailureMsg;
}

// 0x003e1730
Message *GameConnectFailureMsg::Clone() {
    return new GameConnectFailureMsg(*this);
}

// 0x003e17d0
int GameConnectFailureMsg::Type() {
    return g_nGameConnectFailureMsgType;
}

// 0x003e17e0
const char *GameConnectFailureMsg::Name() {
    return "GameConnectFailureMsg";
}

// 0x003e4048
void GameConnectFailureMsg::Print(std::ostream &stream) {
    stream << mUnknown04;
}
