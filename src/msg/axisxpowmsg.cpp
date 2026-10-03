#include "msg/axisxpowmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// NTSC-U/C: 0x003d6aa8, PAL: 0x0040e998
Message *AxisXPowMsg::New() {
    return new AxisXPowMsg;
}

// NTSC-U/C: 0x003db060, PAL: 0x00413498
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *AxisXPowMsg::Clone() {
    return new AxisXPowMsg(*this);
}

// NTSC-U/C: 0x003db0b8, PAL: 0x004134f0
int AxisXPowMsg::Type() {
    return g_nAxisXPowMsgType;
}

// NTSC-U/C: 0x003db0c8, PAL: 0x00413500
const char *AxisXPowMsg::GetName() const {
    return "AxisXPowMsg";
}

// NTSC-U/C: 0x003e3550, PAL: 0x0041b8d0
// The colour name is copied into a temporary before it is written.
void AxisXPowMsg::PrintExtra(std::ostream &stream) const {
    mPosition.Print(stream);
    stream << " " << HxStr(mPlayer->mColorName) << " " << mValue;
}
