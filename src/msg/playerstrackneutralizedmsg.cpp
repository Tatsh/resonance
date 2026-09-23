#include "msg/playerstrackneutralizedmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// 0x003d77a0
Message *PlayersTrackNeutralizedMsg::New() {
    return new PlayersTrackNeutralizedMsg;
}

// 0x003e05b0. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *PlayersTrackNeutralizedMsg::Clone() {
    return new PlayersTrackNeutralizedMsg(*this);
}

// 0x003e0618
int PlayersTrackNeutralizedMsg::Type() {
    return g_nPlayersTrackNeutralizedMsgType;
}

// 0x003e0628
const char *PlayersTrackNeutralizedMsg::Name() {
    return "PlayersTrackNeutralizedMsg";
}

// 0x003e3f30. The colour name is copied into a temporary before it is written.
void PlayersTrackNeutralizedMsg::Print(std::ostream &stream) {
    stream << HxStr(mPlayer->mColorName);
}
