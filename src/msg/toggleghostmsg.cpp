#include "msg/toggleghostmsg.h"

// 0x003d6c28
Message *ToggleGhostMsg::New() {
    return new ToggleGhostMsg;
}

// 0x0011d7c0
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *ToggleGhostMsg::Clone() {
    return new ToggleGhostMsg(*this);
}

// 0x0011d810
int ToggleGhostMsg::Type() {
    return g_nToggleGhostMsgType;
}

// 0x0011d820
const char *ToggleGhostMsg::Name() {
    return "ToggleGhostMsg";
}
