#include "msg/invalidatetrackmsg.h"

#include <iostream>

Message *InvalidateTrackMsg::New() {
    return new InvalidateTrackMsg;
}

Message *InvalidateTrackMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new InvalidateTrackMsg(*this);
}

int InvalidateTrackMsg::Type() {
    return g_nInvalidateTrackMsgType;
}

const char *InvalidateTrackMsg::GetName() const {
    return "InvalidateTrackMsg";
}

void InvalidateTrackMsg::PrintExtra(std::ostream &stream) const {
    stream << "tr#" << mTrack << " song-bars " << mFirstBar << "-" << mEndBar;
}
