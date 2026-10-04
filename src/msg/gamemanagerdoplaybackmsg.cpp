#include "msg/gamemanagerdoplaybackmsg.h"

Message *GameManagerDoPlaybackMsg::New() {
    return new GameManagerDoPlaybackMsg;
}

Message *GameManagerDoPlaybackMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new GameManagerDoPlaybackMsg(*this);
}

int GameManagerDoPlaybackMsg::Type() {
    return g_nGameManagerDoPlaybackMsgType;
}

const char *GameManagerDoPlaybackMsg::GetName() const {
    return "GameManagerDoPlaybackMsg";
}
