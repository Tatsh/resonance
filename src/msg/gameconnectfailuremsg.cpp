#include "msg/gameconnectfailuremsg.h"

#include <iostream>

// NTSC-U/C: 0x003e1810, PAL: 0x00419c80
GameConnectFailureMsg::GameConnectFailureMsg(const HxStr &reason) : mReason(reason) {
}

// NTSC-U/C: 0x003d7a58, PAL: 0x0040f958
Message *GameConnectFailureMsg::New() {
    return new GameConnectFailureMsg;
}

// NTSC-U/C: 0x003e1730, PAL: 0x00419b98
Message *GameConnectFailureMsg::Clone() {
    return new GameConnectFailureMsg(*this);
}

// NTSC-U/C: 0x003e17d0, PAL: 0x00419c38
int GameConnectFailureMsg::Type() {
    return g_nGameConnectFailureMsgType;
}

// NTSC-U/C: 0x003e17e0, PAL: 0x00419c48
const char *GameConnectFailureMsg::Name() {
    return "GameConnectFailureMsg";
}

// NTSC-U/C: 0x003e4048, PAL: 0x0041c278
void GameConnectFailureMsg::Print(std::ostream &stream) {
    stream << mReason;
}
