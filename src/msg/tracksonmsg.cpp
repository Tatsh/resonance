#include "msg/tracksonmsg.h"

#include <iostream>

Message *TracksOnMsg::New() {
    return new TracksOnMsg;
}

Message *TracksOnMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new TracksOnMsg(*this);
}

int TracksOnMsg::Type() {
    return g_dwTracksOnMsgType;
}

const char *TracksOnMsg::GetName() const {
    return "TracksOnMsg";
}

void TracksOnMsg::PrintExtra(std::ostream &stream) const {
    stream << "bar " << mBar << " tracks " << mTracks;
}
