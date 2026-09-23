#include "msg/axisfxmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// 0x003d6a28
Message *AxisFXMsg::New() {
    return new AxisFXMsg;
}

// 0x003dacc8
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *AxisFXMsg::Clone() {
    return new AxisFXMsg(*this);
}

// 0x003dad28
int AxisFXMsg::Type() {
    return g_nAxisFXMsgType;
}

// 0x003dad38
const char *AxisFXMsg::Name() {
    return "AxisFXMsg";
}

// 0x003e3240
// The colour name is copied into a temporary before it is written.
void AxisFXMsg::Print(std::ostream &stream) {
    mPosition.Print(stream);
    stream << " " << HxStr(mPlayer->mColorName) << " " << mValue;
}
