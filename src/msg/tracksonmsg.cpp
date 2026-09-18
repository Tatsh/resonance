#include "msg/tracksonmsg.h"

// 0x003de910. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *TracksOnMsg::Clone() {
    return new TracksOnMsg(*this);
}

// 0x003de960
int TracksOnMsg::Type() {
    return g_nTracksOnMsgType;
}

// 0x003de970
const char *TracksOnMsg::Name() {
    return "TracksOnMsg";
}
