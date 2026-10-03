#include "msg/playerleftgamemsg.h"

#include <iostream>

// NTSC-U/C: 0x003d7b18, PAL: 0x0040fa30
Message *PlayerLeftGameMsg::New() {
    return new PlayerLeftGameMsg;
}

// NTSC-U/C: 0x003e1e88, PAL: 0x0041a328
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *PlayerLeftGameMsg::Clone() {
    return new PlayerLeftGameMsg(*this);
}

// NTSC-U/C: 0x003e1ed0, PAL: 0x0041a370
int PlayerLeftGameMsg::Type() {
    return g_nPlayerLeftGameMsgType;
}

// NTSC-U/C: 0x003e1ee0, PAL: 0x0041a380
const char *PlayerLeftGameMsg::Name() {
    return "PlayerLeftGameMsg";
}

// NTSC-U/C: 0x003e40a0, PAL: 0x0041c2d0
void PlayerLeftGameMsg::Print(std::ostream &stream) {
    stream << mPlayerId;
}
