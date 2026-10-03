#include "msg/invalidatetrackmsg.h"

#include <iostream>

// NTSC-U/C: 0x003d72b8, PAL: 0x0040f1b8
Message *InvalidateTrackMsg::New() {
    return new InvalidateTrackMsg;
}

// NTSC-U/C: 0x003de450, PAL: 0x004168a8
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *InvalidateTrackMsg::Clone() {
    return new InvalidateTrackMsg(*this);
}

// NTSC-U/C: 0x003de4a8, PAL: 0x00416900
int InvalidateTrackMsg::Type() {
    return g_nInvalidateTrackMsgType;
}

// NTSC-U/C: 0x003de4b8, PAL: 0x00416910
const char *InvalidateTrackMsg::GetName() const {
    return "InvalidateTrackMsg";
}

// NTSC-U/C: 0x003e3d90, PAL: 0x0041bf60
void InvalidateTrackMsg::PrintExtra(std::ostream &stream) const {
    stream << "tr#" << mTrack << " song-bars " << mFirstBar << "-" << mEndBar;
}
