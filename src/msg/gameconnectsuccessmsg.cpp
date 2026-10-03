#include "msg/gameconnectsuccessmsg.h"

#include <iostream>

// NTSC-U/C: 0x003d7a20, PAL: 0x0040f920
Message *GameConnectSuccessMsg::New() {
    return new GameConnectSuccessMsg;
}

// NTSC-U/C: 0x003e15a8, PAL: 0x00419a00
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *GameConnectSuccessMsg::Clone() {
    return new GameConnectSuccessMsg(*this);
}

// NTSC-U/C: 0x003e15e0, PAL: 0x00419a38
int GameConnectSuccessMsg::Type() {
    return g_nGameConnectSuccessMsgType;
}

// NTSC-U/C: 0x003e15f0, PAL: 0x00419a48
const char *GameConnectSuccessMsg::GetName() const {
    return "GameConnectSuccessMsg";
}

// NTSC-U/C: 0x003e4040, PAL: 0x0041c270
void GameConnectSuccessMsg::PrintExtra(std::ostream &) const {
}
