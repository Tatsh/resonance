#include "msg/metstartnetlaunchmsg.h"

// 0x003e2960. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *MetStartNetLaunchMsg::Clone() {
    return new MetStartNetLaunchMsg(*this);
}

// 0x003e2998
int MetStartNetLaunchMsg::Type() {
    return g_nMetStartNetLaunchMsgType;
}

// 0x003e29a8
const char *MetStartNetLaunchMsg::Name() {
    return "MetStartNetLaunchMsg";
}
