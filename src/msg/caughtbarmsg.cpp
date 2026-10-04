#include "msg/caughtbarmsg.h"

Message *CaughtBarMsg::New() {
    return new CaughtBarMsg;
}

Message *CaughtBarMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new CaughtBarMsg(*this);
}

int CaughtBarMsg::Type() {
    return g_nCaughtBarMsgType;
}

const char *CaughtBarMsg::GetName() const {
    return "CaughtBarMsg";
}
