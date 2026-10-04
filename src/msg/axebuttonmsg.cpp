#include "msg/axebuttonmsg.h"

Message *AxeButtonMsg::New() {
    return new AxeButtonMsg;
}

Message *AxeButtonMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new AxeButtonMsg(*this);
}

int AxeButtonMsg::Type() {
    return g_nAxeButtonMsgType;
}

const char *AxeButtonMsg::GetName() const {
    return "AxeButtonMsg";
}
