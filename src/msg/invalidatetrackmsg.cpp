#include "msg/invalidatetrackmsg.h"

#include <iostream>

// 0x003d72b8
Message *InvalidateTrackMsg::New() {
    return new InvalidateTrackMsg;
}

// 0x003de450. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *InvalidateTrackMsg::Clone() {
    return new InvalidateTrackMsg(*this);
}

// 0x003de4a8
int InvalidateTrackMsg::Type() {
    return g_nInvalidateTrackMsgType;
}

// 0x003de4b8
const char *InvalidateTrackMsg::Name() {
    return "InvalidateTrackMsg";
}

// 0x003e3d90
void InvalidateTrackMsg::Print(std::ostream &stream) {
    stream << "tr#" << mTrack << " song-bars " << mFirstBar << "-" << mEndBar;
}
