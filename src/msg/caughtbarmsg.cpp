#include "msg/caughtbarmsg.h"

// 0x003d6d08
Message *CaughtBarMsg::New() {
    return new CaughtBarMsg;
}

// 0x001b11b0. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *CaughtBarMsg::Clone() {
    return new CaughtBarMsg(*this);
}

// 0x001b1200
int CaughtBarMsg::Type() {
    return g_nCaughtBarMsgType;
}

// 0x001b1210
const char *CaughtBarMsg::Name() {
    return "CaughtBarMsg";
}
