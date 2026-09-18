#include "msg/trackselectmsg.h"

// 0x003dc930. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *TrackSelectMsg::Clone() {
    return new TrackSelectMsg(*this);
}

// 0x003dc990
int TrackSelectMsg::Type() {
    return g_nTrackSelectMsgType;
}

// 0x003dc9a0
const char *TrackSelectMsg::Name() {
    return "TrackSelectMsg";
}
