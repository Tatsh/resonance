#include "msg/nearesttrackmsg.h"

#include <iostream>

Message *NearestTrackMsg::New() {
    return new NearestTrackMsg;
}

Message *NearestTrackMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new NearestTrackMsg(*this);
}

int NearestTrackMsg::Type() {
    return g_nNearestTrackMsgType;
}

const char *NearestTrackMsg::GetName() const {
    return "NearestTrackMsg";
}

void NearestTrackMsg::PrintExtra(std::ostream &stream) const {
    if (mTrack == -1) {
        stream << "reset";
    } else {
        stream << mTrack;
    }
}
