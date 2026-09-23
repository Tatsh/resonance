#include "msg/tracksonmsg.h"

#include <iostream>

// 0x003d7360
Message *TracksOnMsg::New() {
    return new TracksOnMsg;
}

// 0x003de910. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *TracksOnMsg::Clone() {
    return new TracksOnMsg(*this);
}

// 0x003de960
int TracksOnMsg::Type() {
    return g_dwTracksOnMsgType;
}

// 0x003de970
const char *TracksOnMsg::Name() {
    return "TracksOnMsg";
}

// 0x003e4408
void TracksOnMsg::Print(std::ostream &stream) {
    stream << "bar " << mBar << " tracks " << mTracks;
}
