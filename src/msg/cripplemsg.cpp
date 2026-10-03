#include "msg/cripplemsg.h"

#include <iostream>

#include "game/player.h"

// NTSC-U/C: 0x003d7ca0, PAL: 0x0040fbb8
Message *CrippleMsg::New() {
    return new CrippleMsg;
}

// NTSC-U/C: 0x003e2788, PAL: 0x0041ac28
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *CrippleMsg::Clone() {
    return new CrippleMsg(*this);
}

// NTSC-U/C: 0x003e27f8, PAL: 0x0041ac98
int CrippleMsg::Type() {
    return sID;
}

// NTSC-U/C: 0x003e2808, PAL: 0x0041aca8
const char *CrippleMsg::GetName() const {
    return "CrippleMsg";
}

// NTSC-U/C: 0x003e4290, PAL: 0x0041c4c0
void CrippleMsg::PrintExtra(std::ostream &stream) const {
    stream << "tr#" << mTrack;
    stream << " p#" << mPlayer->mPlayerId;
}
