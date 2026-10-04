#include "msg/autocatchmsg.h"

#include <iostream>

#include "game/player.h"

Message *AutoCatchMsg::New() {
    return new AutoCatchMsg;
}

Message *AutoCatchMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new AutoCatchMsg(*this);
}

int AutoCatchMsg::Type() {
    return g_nAutoCatchMsgType;
}

const char *AutoCatchMsg::GetName() const {
    return "AutoCatchMsg";
}

void AutoCatchMsg::PrintExtra(std::ostream &stream) const {
    stream << "tr#" << mTrack;
    stream << " p#" << mPlayer->mPlayerId << " " << mBar;
}
