#include "msg/sectioncapturedmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// NTSC-U/C: 0x003d7408, PAL: 0x0040f308
Message *SectionCapturedMsg::New() {
    return new SectionCapturedMsg;
}

// NTSC-U/C: 0x003dee18, PAL: 0x00417270
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *SectionCapturedMsg::Clone() {
    return new SectionCapturedMsg(*this);
}

// NTSC-U/C: 0x003dee80, PAL: 0x004172d8
int SectionCapturedMsg::Type() {
    return g_nSectionCapturedMsgType;
}

// NTSC-U/C: 0x003dee90, PAL: 0x004172e8
const char *SectionCapturedMsg::Name() {
    return "SectionCapturedMsg";
}

// NTSC-U/C: 0x003d8578, PAL: 0x00410950
// The colour name is copied into a temporary before it is written.
void SectionCapturedMsg::Print(std::ostream &stream) {
    stream << "b " << mFirstBar << "--" << mEndBar << " tr# " << mTrack;
    stream << " " << HxStr(mPlayer->mColorName);
}
