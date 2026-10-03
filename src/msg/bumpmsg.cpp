#include "msg/bumpmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// NTSC-U/C: 0x003d79e8, PAL: 0x0040f8e8
Message *BumpMsg::New() {
    return new BumpMsg;
}

// NTSC-U/C: 0x003e13d0, PAL: 0x00419828
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *BumpMsg::Clone() {
    return new BumpMsg(*this);
}

// NTSC-U/C: 0x003e1440, PAL: 0x00419898
int BumpMsg::Type() {
    return g_nBumpMsgType;
}

// NTSC-U/C: 0x003e1450, PAL: 0x004198a8
const char *BumpMsg::Name() {
    return "BumpMsg";
}

// NTSC-U/C: 0x003e3fb8, PAL: 0x0041c1c8
// The colour name is copied into a temporary before it is written.
void BumpMsg::Print(std::ostream &stream) {
    stream << HxStr(mPlayer->mColorName);
}
