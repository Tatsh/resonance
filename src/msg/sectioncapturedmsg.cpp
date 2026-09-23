#include "msg/sectioncapturedmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// 0x003d7408
Message *SectionCapturedMsg::New() {
    return new SectionCapturedMsg;
}

// 0x003dee18. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *SectionCapturedMsg::Clone() {
    return new SectionCapturedMsg(*this);
}

// 0x003dee80
int SectionCapturedMsg::Type() {
    return g_nSectionCapturedMsgType;
}

// 0x003dee90
const char *SectionCapturedMsg::Name() {
    return "SectionCapturedMsg";
}

// 0x003d8578. The colour name is copied into a temporary before it is written.
void SectionCapturedMsg::Print(std::ostream &stream) {
    stream << "b " << mUnknown04 << "--" << mUnknown08 << " tr# " << mTrack;
    stream << " " << HxStr(mPlayer->mColorName);
}
