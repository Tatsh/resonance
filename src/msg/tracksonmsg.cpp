#include "msg/tracksonmsg.h"

#include <iostream>

// NTSC-U/C: 0x003d7360, PAL: 0x0040f260
Message *TracksOnMsg::New() {
    return new TracksOnMsg;
}

// NTSC-U/C: 0x003de910, PAL: 0x00416d68
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *TracksOnMsg::Clone() {
    return new TracksOnMsg(*this);
}

// NTSC-U/C: 0x003de960, PAL: 0x00416db8
int TracksOnMsg::Type() {
    return g_dwTracksOnMsgType;
}

// NTSC-U/C: 0x003de970, PAL: 0x00416dc8
const char *TracksOnMsg::GetName() const {
    return "TracksOnMsg";
}

// NTSC-U/C: 0x003e4408, PAL: 0x0041c638
void TracksOnMsg::PrintExtra(std::ostream &stream) const {
    stream << "bar " << mBar << " tracks " << mTracks;
}
