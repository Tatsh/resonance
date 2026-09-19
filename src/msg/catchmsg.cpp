#include "msg/catchmsg.h"

// 0x003d75c0
Message *CatchMsg::New() {
    return new CatchMsg;
}

// 0x001b1068. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *CatchMsg::Clone() {
    return new CatchMsg(*this);
}

// 0x001b10e0
int CatchMsg::Type() {
    return g_nCatchMsgType;
}

// 0x001b10f0
const char *CatchMsg::Name() {
    return "CatchMsg";
}
