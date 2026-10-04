#include "msg/sectioncapturedmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

Message *SectionCapturedMsg::New() {
    return new SectionCapturedMsg;
}

Message *SectionCapturedMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new SectionCapturedMsg(*this);
}

int SectionCapturedMsg::Type() {
    return g_nSectionCapturedMsgType;
}

const char *SectionCapturedMsg::GetName() const {
    return "SectionCapturedMsg";
}

void SectionCapturedMsg::PrintExtra(std::ostream &stream) const {
    stream << "b " << mFirstBar << "--" << mEndBar << " tr# " << mTrack;
    // The colour name is copied into a temporary before it is written.
    stream << " " << HxStr(mPlayer->mColorName);
}
