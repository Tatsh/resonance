#include "msg/metstartpausemsg.h"

// 0x003e2ac0. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *MetStartPauseMsg::Clone() {
    return new MetStartPauseMsg(*this);
}

// 0x003e2af8
int MetStartPauseMsg::Type() {
    return g_nMetStartPauseMsgType;
}

// 0x003e2b08
const char *MetStartPauseMsg::Name() {
    return "MetStartPauseMsg";
}
