#include "msg/playerstrackneutralizedmsg.h"

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
