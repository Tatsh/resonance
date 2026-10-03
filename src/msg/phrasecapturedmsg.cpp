#include "msg/phrasecapturedmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// NTSC-U/C: 0x003d73d0, PAL: 0x0040f2d0
Message *PhraseCapturedMsg::New() {
    return new PhraseCapturedMsg;
}

// NTSC-U/C: 0x003debe0, PAL: 0x00417038
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *PhraseCapturedMsg::Clone() {
    return new PhraseCapturedMsg(*this);
}

// NTSC-U/C: 0x003dec68, PAL: 0x004170c0
int PhraseCapturedMsg::Type() {
    return g_nPhraseCapturedMsgType;
}

// NTSC-U/C: 0x003dec78, PAL: 0x004170d0
const char *PhraseCapturedMsg::Name() {
    return "PhraseCapturedMsg";
}

// NTSC-U/C: 0x003d8448, PAL: 0x00410800
// The colour name is copied into a temporary before it is written.
void PhraseCapturedMsg::Print(std::ostream &stream) {
    stream << "b " << mFirstBar << "--" << mEndBar << " tr# " << mTrack;
    stream << " score " << mScore << " juice " << mJuice << " " << HxStr(mPlayer->mColorName);
}
