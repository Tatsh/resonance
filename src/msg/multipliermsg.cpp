#include "msg/multipliermsg.h"

Message *MultiplierMsg::New() {
    return new MultiplierMsg;
}

Message *MultiplierMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new MultiplierMsg(*this);
}

int MultiplierMsg::Type() {
    return g_nMultiplierMsgType;
}

const char *MultiplierMsg::GetName() const {
    return "MultiplierMsg";
}
