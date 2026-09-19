#include "msg/rotleftmsg.h"

// 0x003d68a8
Message *RotLeftMsg::New() {
    return new RotLeftMsg;
}

// 0x0011d338. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *RotLeftMsg::Clone() {
    return new RotLeftMsg(*this);
}

// 0x0011d388
int RotLeftMsg::Type() {
    return g_nRotLeftMsgType;
}

// 0x0011d398
const char *RotLeftMsg::Name() {
    return "RotLeftMsg";
}
