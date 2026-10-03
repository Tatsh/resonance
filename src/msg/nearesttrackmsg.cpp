#include "msg/nearesttrackmsg.h"

#include <iostream>

// NTSC-U/C: 0x003d70a8, PAL: 0x0040ef98
Message *NearestTrackMsg::New() {
    return new NearestTrackMsg;
}

// NTSC-U/C: 0x003dd7c0, PAL: 0x00415bf8
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *NearestTrackMsg::Clone() {
    return new NearestTrackMsg(*this);
}

// NTSC-U/C: 0x003dd808, PAL: 0x00415c40
int NearestTrackMsg::Type() {
    return g_nNearestTrackMsgType;
}

// NTSC-U/C: 0x003dd818, PAL: 0x00415c50
const char *NearestTrackMsg::Name() {
    return "NearestTrackMsg";
}

// NTSC-U/C: 0x003e3d50, PAL: 0x0041bf20
void NearestTrackMsg::Print(std::ostream &stream) {
    if (mTrack == -1) {
        stream << "reset";
    } else {
        stream << mTrack;
    }
}
