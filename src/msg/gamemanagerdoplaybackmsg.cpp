#include "msg/gamemanagerdoplaybackmsg.h"

// NTSC-U/C: 0x003d7c30, PAL: 0x0040fb48
Message *GameManagerDoPlaybackMsg::New() {
    return new GameManagerDoPlaybackMsg;
}

// NTSC-U/C: 0x00291970, PAL: 0x002ad888
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *GameManagerDoPlaybackMsg::Clone() {
    return new GameManagerDoPlaybackMsg(*this);
}

// NTSC-U/C: 0x002919a8, PAL: 0x002ad8c0
int GameManagerDoPlaybackMsg::Type() {
    return g_nGameManagerDoPlaybackMsgType;
}

// NTSC-U/C: 0x002919b8, PAL: 0x002ad8d0
const char *GameManagerDoPlaybackMsg::GetName() const {
    return "GameManagerDoPlaybackMsg";
}
