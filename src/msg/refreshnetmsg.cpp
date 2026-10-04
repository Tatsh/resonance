#include "msg/refreshnetmsg.h"

#include <iostream>

Message *RefreshNetMsg::New() {
    return new RefreshNetMsg;
}

Message *RefreshNetMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new RefreshNetMsg(*this);
}

int RefreshNetMsg::Type() {
    return g_nRefreshNetMsgType;
}

const char *RefreshNetMsg::GetName() const {
    return "RefreshNetMsg";
}

void RefreshNetMsg::PrintExtra(std::ostream &stream) const {
    stream << "tr#" << mTrack << " bars " << mFirstBar << " - " << mEndBar;
}
