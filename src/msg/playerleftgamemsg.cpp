#include "msg/playerleftgamemsg.h"

#include <iostream>

Message *PlayerLeftGameMsg::New() {
    return new PlayerLeftGameMsg;
}

Message *PlayerLeftGameMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new PlayerLeftGameMsg(*this);
}

int PlayerLeftGameMsg::Type() {
    return g_nPlayerLeftGameMsgType;
}

const char *PlayerLeftGameMsg::GetName() const {
    return "PlayerLeftGameMsg";
}

void PlayerLeftGameMsg::PrintExtra(std::ostream &stream) const {
    stream << mPlayerId;
}
