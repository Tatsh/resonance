#include "msg/playerstrackneutralizedmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// NTSC-U/C: 0x003d77a0, PAL: 0x0040f6a0
Message *PlayersTrackNeutralizedMsg::New() {
    return new PlayersTrackNeutralizedMsg;
}

// NTSC-U/C: 0x003e05b0, PAL: 0x00418a08
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *PlayersTrackNeutralizedMsg::Clone() {
    return new PlayersTrackNeutralizedMsg(*this);
}

// NTSC-U/C: 0x003e0618, PAL: 0x00418a70
int PlayersTrackNeutralizedMsg::Type() {
    return g_nPlayersTrackNeutralizedMsgType;
}

// NTSC-U/C: 0x003e0628, PAL: 0x00418a80
const char *PlayersTrackNeutralizedMsg::GetName() const {
    return "PlayersTrackNeutralizedMsg";
}

// NTSC-U/C: 0x003e3f30, PAL: 0x0041c120
// The colour name is copied into a temporary before it is written.
void PlayersTrackNeutralizedMsg::PrintExtra(std::ostream &stream) const {
    stream << HxStr(mPlayer->mColorName);
}
