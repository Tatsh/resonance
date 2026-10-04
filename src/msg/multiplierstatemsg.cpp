#include "msg/multiplierstatemsg.h"

Message *MultiplierStateMsg::New() {
    return new MultiplierStateMsg;
}

Message *MultiplierStateMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new MultiplierStateMsg(*this);
}

int MultiplierStateMsg::Type() {
    return g_nMultiplierStateMsgType;
}

const char *MultiplierStateMsg::GetName() const {
    return "MultiplierStateMsg";
}
