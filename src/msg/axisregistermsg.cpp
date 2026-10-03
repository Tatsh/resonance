#include "msg/axisregistermsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// NTSC-U/C: 0x003d69e8, PAL: 0x0040e8d8
Message *AxisRegisterMsg::New() {
    return new AxisRegisterMsg;
}

// NTSC-U/C: 0x003daaf0, PAL: 0x00412f28
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *AxisRegisterMsg::Clone() {
    return new AxisRegisterMsg(*this);
}

// NTSC-U/C: 0x003dab50, PAL: 0x00412f88
int AxisRegisterMsg::Type() {
    return g_nAxisRegisterMsgType;
}

// NTSC-U/C: 0x003dab60, PAL: 0x00412f98
const char *AxisRegisterMsg::Name() {
    return "AxisRegisterMsg";
}

// NTSC-U/C: 0x003e3160, PAL: 0x0040fd08
// The colour name is copied into a temporary before it is written.
void AxisRegisterMsg::Print(std::ostream &stream) {
    mPosition.Print(stream);
    stream << " " << HxStr(mPlayer->mColorName) << " " << mValue;
}
