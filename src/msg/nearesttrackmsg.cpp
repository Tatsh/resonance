#include "msg/nearesttrackmsg.h"

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
