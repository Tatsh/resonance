#include "msg/axisypowmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// 0x003d6a68
Message *AxisYPowMsg::New() {
    return new AxisYPowMsg;
}

// 0x003daea0
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *AxisYPowMsg::Clone() {
    return new AxisYPowMsg(*this);
}

// 0x003daef8
int AxisYPowMsg::Type() {
    return g_nAxisYPowMsgType;
}

// 0x003daf08
const char *AxisYPowMsg::Name() {
    return "AxisYPowMsg";
}

// 0x003e3480
// The colour name is copied into a temporary before it is written.
void AxisYPowMsg::Print(std::ostream &stream) {
    mPosition.Print(stream);
    stream << " " << HxStr(mPlayer->mColorName) << " " << mValue;
}
