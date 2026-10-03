#include "msg/phrasemuffedmsg.h"

#include <iostream>

#include "game/player.h"

// NTSC-U/C: 0x003d76f0, PAL: 0x0040f5f0
Message *PhraseMuffedMsg::New() {
    return new PhraseMuffedMsg;
}

// NTSC-U/C: 0x003e0038, PAL: 0x00418490
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *PhraseMuffedMsg::Clone() {
    return new PhraseMuffedMsg(*this);
}

// NTSC-U/C: 0x003e0098, PAL: 0x004184f0
int PhraseMuffedMsg::Type() {
    return g_nPhraseMuffedMsgType;
}

// NTSC-U/C: 0x003e00a8, PAL: 0x00418500
const char *PhraseMuffedMsg::GetName() const {
    return "PhraseMuffedMsg";
}

// NTSC-U/C: 0x003e4350, PAL: 0x0041c580
void PhraseMuffedMsg::PrintExtra(std::ostream &stream) const {
    std::ostream &rest = stream << "tr#" << mTrack << " ";
    mPlayer->Print(rest);
    std::ostream &tail = rest << " ";
    mPosition.Print(tail);
    tail << " tried:" << mTried;
}
