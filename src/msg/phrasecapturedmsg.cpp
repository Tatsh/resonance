#include "msg/phrasecapturedmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// 0x003d73d0
Message *PhraseCapturedMsg::New() {
    return new PhraseCapturedMsg;
}

// 0x003debe0
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *PhraseCapturedMsg::Clone() {
    return new PhraseCapturedMsg(*this);
}

// 0x003dec68
int PhraseCapturedMsg::Type() {
    return g_nPhraseCapturedMsgType;
}

// 0x003dec78
const char *PhraseCapturedMsg::Name() {
    return "PhraseCapturedMsg";
}

// 0x003d8448
// The colour name is copied into a temporary before it is written.
void PhraseCapturedMsg::Print(std::ostream &stream) {
    stream << "b " << mFirstBar << "--" << mEndBar << " tr# " << mTrack;
    stream << " score " << mScore << " juice " << mJuice << " " << HxStr(mPlayer->mColorName);
}
