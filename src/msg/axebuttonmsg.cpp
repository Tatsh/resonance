#include "msg/axebuttonmsg.h"

// 0x003d76b8
Message *AxeButtonMsg::New() {
    return new AxeButtonMsg;
}

// 0x0019a748. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *AxeButtonMsg::Clone() {
    return new AxeButtonMsg(*this);
}

// 0x0019a7a0
int AxeButtonMsg::Type() {
    return g_nAxeButtonMsgType;
}

// 0x0019a7b0
const char *AxeButtonMsg::Name() {
    return "AxeButtonMsg";
}
