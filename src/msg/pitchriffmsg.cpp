#include "msg/pitchriffmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// NTSC-U/C: 0x003d6928, PAL: 0x0040e818
Message *PitchRiffMsg::New() {
    return new PitchRiffMsg;
}

// NTSC-U/C: 0x003da620, PAL: 0x00412a58
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *PitchRiffMsg::Clone() {
    return new PitchRiffMsg(*this);
}

// NTSC-U/C: 0x003da680, PAL: 0x00412ab8
int PitchRiffMsg::Type() {
    return g_nPitchRiffMsgType;
}

// NTSC-U/C: 0x003da690, PAL: 0x00412ac8
const char *PitchRiffMsg::Name() {
    return "PitchRiffMsg";
}

// NTSC-U/C: 0x003e2fd0, PAL: 0x0041b470
// The colour name is copied into a temporary before it is written.
void PitchRiffMsg::Print(std::ostream &stream) {
    mPosition.Print(stream);
    stream << " " << HxStr(mPlayer->mColorName) << " b#" << mButton;
}
