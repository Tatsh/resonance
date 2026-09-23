#include "msg/axisxpowmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// 0x003d6aa8
Message *AxisXPowMsg::New() {
    return new AxisXPowMsg;
}

// 0x003db060. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *AxisXPowMsg::Clone() {
    return new AxisXPowMsg(*this);
}

// 0x003db0b8
int AxisXPowMsg::Type() {
    return g_nAxisXPowMsgType;
}

// 0x003db0c8
const char *AxisXPowMsg::Name() {
    return "AxisXPowMsg";
}

// 0x003e3550. The colour name is copied into a temporary before it is written.
void AxisXPowMsg::Print(std::ostream &stream) {
    mPosition.Print(stream);
    stream << " " << HxStr(mPlayer->mColorName) << " " << mUnknown08;
}
