#include "msg/axisregistermsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// 0x003d69e8
Message *AxisRegisterMsg::New() {
    return new AxisRegisterMsg;
}

// 0x003daaf0
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *AxisRegisterMsg::Clone() {
    return new AxisRegisterMsg(*this);
}

// 0x003dab50
int AxisRegisterMsg::Type() {
    return g_nAxisRegisterMsgType;
}

// 0x003dab60
const char *AxisRegisterMsg::Name() {
    return "AxisRegisterMsg";
}

// 0x003e3160
// The colour name is copied into a temporary before it is written.
void AxisRegisterMsg::Print(std::ostream &stream) {
    mPosition.Print(stream);
    stream << " " << HxStr(mPlayer->mColorName) << " " << mValue;
}
