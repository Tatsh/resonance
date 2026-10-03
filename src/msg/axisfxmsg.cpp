#include "msg/axisfxmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// NTSC-U/C: 0x003d6a28, PAL: 0x0040e918
Message *AxisFXMsg::New() {
    return new AxisFXMsg;
}

// NTSC-U/C: 0x003dacc8, PAL: 0x00413100
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *AxisFXMsg::Clone() {
    return new AxisFXMsg(*this);
}

// NTSC-U/C: 0x003dad28, PAL: 0x00413160
int AxisFXMsg::Type() {
    return g_nAxisFXMsgType;
}

// NTSC-U/C: 0x003dad38, PAL: 0x00413170
const char *AxisFXMsg::Name() {
    return "AxisFXMsg";
}

// NTSC-U/C: 0x003e3240, PAL: 0x0040fe08
// The colour name is copied into a temporary before it is written.
void AxisFXMsg::Print(std::ostream &stream) {
    mPosition.Print(stream);
    stream << " " << HxStr(mPlayer->mColorName) << " " << mValue;
}
