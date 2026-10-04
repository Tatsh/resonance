#include "msg/freestylefxmsg.h"

Message *FreestyleFXMsg::New() {
    return new FreestyleFXMsg;
}

Message *FreestyleFXMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new FreestyleFXMsg(*this);
}

int FreestyleFXMsg::Type() {
    return g_nFreestyleFXMsgType;
}

const char *FreestyleFXMsg::GetName() const {
    return "FreestyleFXMsg";
}
