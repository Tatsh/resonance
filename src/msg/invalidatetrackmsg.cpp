#include "msg/invalidatetrackmsg.h"

// 0x003de450. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *InvalidateTrackMsg::Clone() {
    return new InvalidateTrackMsg(*this);
}

// 0x003de4a8
int InvalidateTrackMsg::Type() {
    return g_nInvalidateTrackMsgType;
}

// 0x003de4b8
const char *InvalidateTrackMsg::Name() {
    return "InvalidateTrackMsg";
}
