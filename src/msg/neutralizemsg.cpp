#include "msg/neutralizemsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// NTSC-U/C: 0x003d7768, PAL: 0x0040f668
Message *NeutralizeMsg::New() {
    return new NeutralizeMsg;
}

// NTSC-U/C: 0x003e03a8, PAL: 0x00418800
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *NeutralizeMsg::Clone() {
    return new NeutralizeMsg(*this);
}

// NTSC-U/C: 0x003e0418, PAL: 0x00418870
int NeutralizeMsg::Type() {
    return sID;
}

// NTSC-U/C: 0x003e0428, PAL: 0x00418880
const char *NeutralizeMsg::GetName() const {
    return "NeutralizeMsg";
}

// NTSC-U/C: 0x003e3ea8, PAL: 0x0041c078
// The colour name is copied into a temporary before it is written.
void NeutralizeMsg::PrintExtra(std::ostream &stream) const {
    stream << HxStr(mPlayer->mColorName);
}
