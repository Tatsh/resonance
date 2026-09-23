#include "msg/nearesttrackmsg.h"

#include <iostream>

// 0x003d70a8
Message *NearestTrackMsg::New() {
    return new NearestTrackMsg;
}

// 0x003dd7c0. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *NearestTrackMsg::Clone() {
    return new NearestTrackMsg(*this);
}

// 0x003dd808
int NearestTrackMsg::Type() {
    return g_nNearestTrackMsgType;
}

// 0x003dd818
const char *NearestTrackMsg::Name() {
    return "NearestTrackMsg";
}

// 0x003e3d50
void NearestTrackMsg::Print(std::ostream &stream) {
    if (mUnknown04 == -1) {
        stream << "reset";
    } else {
        stream << mUnknown04;
    }
}
