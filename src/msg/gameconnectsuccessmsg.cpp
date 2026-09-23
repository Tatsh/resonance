#include "msg/gameconnectsuccessmsg.h"

#include <iostream>

// 0x003d7a20
Message *GameConnectSuccessMsg::New() {
    return new GameConnectSuccessMsg;
}

// 0x003e15a8
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *GameConnectSuccessMsg::Clone() {
    return new GameConnectSuccessMsg(*this);
}

// 0x003e15e0
int GameConnectSuccessMsg::Type() {
    return g_nGameConnectSuccessMsgType;
}

// 0x003e15f0
const char *GameConnectSuccessMsg::Name() {
    return "GameConnectSuccessMsg";
}

// 0x003e4040
void GameConnectSuccessMsg::Print(std::ostream &) {
}
