#include "msg/bumpmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

Message *BumpMsg::New() {
    return new BumpMsg;
}

Message *BumpMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new BumpMsg(*this);
}

int BumpMsg::Type() {
    return g_nBumpMsgType;
}

const char *BumpMsg::GetName() const {
    return "BumpMsg";
}

void BumpMsg::PrintExtra(std::ostream &stream) const {
    // The colour name is copied into a temporary before it is written.
    stream << HxStr(mPlayer->mColorName);
}
