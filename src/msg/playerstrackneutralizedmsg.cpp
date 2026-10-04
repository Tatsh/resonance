#include "msg/playerstrackneutralizedmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

Message *PlayersTrackNeutralizedMsg::New() {
    return new PlayersTrackNeutralizedMsg;
}

Message *PlayersTrackNeutralizedMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new PlayersTrackNeutralizedMsg(*this);
}

int PlayersTrackNeutralizedMsg::Type() {
    return g_nPlayersTrackNeutralizedMsgType;
}

const char *PlayersTrackNeutralizedMsg::GetName() const {
    return "PlayersTrackNeutralizedMsg";
}

void PlayersTrackNeutralizedMsg::PrintExtra(std::ostream &stream) const {
    // The colour name is copied into a temporary before it is written.
    stream << HxStr(mPlayer->mColorName);
}
