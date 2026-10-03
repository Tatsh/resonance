#include "msg/axisypowmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// NTSC-U/C: 0x003d6a68, PAL: 0x0040e958
Message *AxisYPowMsg::New() {
    return new AxisYPowMsg;
}

// NTSC-U/C: 0x003daea0, PAL: 0x004132d8
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *AxisYPowMsg::Clone() {
    return new AxisYPowMsg(*this);
}

// NTSC-U/C: 0x003daef8, PAL: 0x00413330
int AxisYPowMsg::Type() {
    return g_nAxisYPowMsgType;
}

// NTSC-U/C: 0x003daf08, PAL: 0x00413340
const char *AxisYPowMsg::GetName() const {
    return "AxisYPowMsg";
}

// NTSC-U/C: 0x003e3480, PAL: 0x0041b7e0
// The colour name is copied into a temporary before it is written.
void AxisYPowMsg::PrintExtra(std::ostream &stream) const {
    mPosition.Print(stream);
    stream << " " << HxStr(mPlayer->mColorName) << " " << mValue;
}
