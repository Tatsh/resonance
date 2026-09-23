#include "msg/bumpmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// 0x003d79e8
Message *BumpMsg::New() {
    return new BumpMsg;
}

// 0x003e13d0
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *BumpMsg::Clone() {
    return new BumpMsg(*this);
}

// 0x003e1440
int BumpMsg::Type() {
    return g_nBumpMsgType;
}

// 0x003e1450
const char *BumpMsg::Name() {
    return "BumpMsg";
}

// 0x003e3fb8
// The colour name is copied into a temporary before it is written.
void BumpMsg::Print(std::ostream &stream) {
    stream << HxStr(mPlayer->mColorName);
}
