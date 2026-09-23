#include "msg/gamemanagerdoplaybackmsg.h"

// 0x003d7c30
Message *GameManagerDoPlaybackMsg::New() {
    return new GameManagerDoPlaybackMsg;
}

// 0x00291970
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *GameManagerDoPlaybackMsg::Clone() {
    return new GameManagerDoPlaybackMsg(*this);
}

// 0x002919a8
int GameManagerDoPlaybackMsg::Type() {
    return g_nGameManagerDoPlaybackMsgType;
}

// 0x002919b8
const char *GameManagerDoPlaybackMsg::Name() {
    return "GameManagerDoPlaybackMsg";
}
