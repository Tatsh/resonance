#include "msg/pitchriffmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// 0x003d6928
Message *PitchRiffMsg::New() {
    return new PitchRiffMsg;
}

// 0x003da620
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *PitchRiffMsg::Clone() {
    return new PitchRiffMsg(*this);
}

// 0x003da680
int PitchRiffMsg::Type() {
    return g_nPitchRiffMsgType;
}

// 0x003da690
const char *PitchRiffMsg::Name() {
    return "PitchRiffMsg";
}

// 0x003e2fd0
// The colour name is copied into a temporary before it is written.
void PitchRiffMsg::Print(std::ostream &stream) {
    mUnknown0c.Print(stream);
    stream << " " << HxStr(mUnknown08->mColorName) << " b#" << mUnknown04;
}
