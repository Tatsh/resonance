#include "msg/refreshnetmsg.h"

#include <iostream>

// NTSC-U/C: 0x003d72f0, PAL: 0x0040f1f0
Message *RefreshNetMsg::New() {
    return new RefreshNetMsg;
}

// NTSC-U/C: 0x003de630, PAL: 0x00416a88
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *RefreshNetMsg::Clone() {
    return new RefreshNetMsg(*this);
}

// NTSC-U/C: 0x003de688, PAL: 0x00416ae0
int RefreshNetMsg::Type() {
    return g_nRefreshNetMsgType;
}

// NTSC-U/C: 0x003de698, PAL: 0x00416af0
const char *RefreshNetMsg::GetName() const {
    return "RefreshNetMsg";
}

// NTSC-U/C: 0x003e3e08, PAL: 0x0041bfd8
void RefreshNetMsg::PrintExtra(std::ostream &stream) const {
    stream << "tr#" << mTrack << " bars " << mFirstBar << " - " << mEndBar;
}
