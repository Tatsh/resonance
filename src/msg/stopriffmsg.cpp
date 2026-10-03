#include "msg/stopriffmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// NTSC-U/C: 0x003d6968, PAL: 0x0040e858
Message *StopRiffMsg::New() {
    return new StopRiffMsg;
}

// NTSC-U/C: 0x003da7f8, PAL: 0x00412c30
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *StopRiffMsg::Clone() {
    return new StopRiffMsg(*this);
}

// NTSC-U/C: 0x003da858, PAL: 0x00412c90
int StopRiffMsg::Type() {
    return sID;
}

// NTSC-U/C: 0x003da868, PAL: 0x00412ca0
const char *StopRiffMsg::GetName() const {
    return "StopRiffMsg";
}

// NTSC-U/C: 0x003e3098, PAL: 0x0041b558
// The colour name is copied into a temporary before it is written.
void StopRiffMsg::PrintExtra(std::ostream &stream) const {
    mPosition.Print(stream);
    stream << " " << HxStr(mPlayer->mColorName) << " b#" << mButton;
}
