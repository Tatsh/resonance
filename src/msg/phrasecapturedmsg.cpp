#include "msg/phrasecapturedmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

Message *PhraseCapturedMsg::New() {
    return new PhraseCapturedMsg;
}

Message *PhraseCapturedMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new PhraseCapturedMsg(*this);
}

int PhraseCapturedMsg::Type() {
    return g_nPhraseCapturedMsgType;
}

const char *PhraseCapturedMsg::GetName() const {
    return "PhraseCapturedMsg";
}

void PhraseCapturedMsg::PrintExtra(std::ostream &stream) const {
    stream << "b " << mFirstBar << "--" << mEndBar << " tr# " << mTrack;
    // The colour name is copied into a temporary before it is written.
    stream << " score " << mScore << " juice " << mJuice << " " << HxStr(mPlayer->mColorName);
}
