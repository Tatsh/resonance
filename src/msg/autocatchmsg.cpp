#include "msg/autocatchmsg.h"

#include <iostream>

#include "game/player.h"

// NTSC-U/C: 0x003d7c68, PAL: 0x0040fb80
Message *AutoCatchMsg::New() {
    return new AutoCatchMsg;
}

// NTSC-U/C: 0x003e2580, PAL: 0x0041aa20
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *AutoCatchMsg::Clone() {
    return new AutoCatchMsg(*this);
}

// NTSC-U/C: 0x003e25f0, PAL: 0x0041aa90
int AutoCatchMsg::Type() {
    return g_nAutoCatchMsgType;
}

// NTSC-U/C: 0x003e2600, PAL: 0x0041aaa0
const char *AutoCatchMsg::Name() {
    return "AutoCatchMsg";
}

// NTSC-U/C: 0x003e4208, PAL: 0x0041c438
void AutoCatchMsg::Print(std::ostream &stream) {
    stream << "tr#" << mTrack;
    stream << " p#" << mPlayer->mPlayerId << " " << mBar;
}
