#include "msg/catchmsg.h"

Message *CatchMsg::New() {
    return new CatchMsg;
}

Message *CatchMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new CatchMsg(*this);
}

int CatchMsg::Type() {
    return g_nCatchMsgType;
}

const char *CatchMsg::GetName() const {
    return "CatchMsg";
}
