#include "msg/rotrightmsg.h"

// 0x003d68e8
Message *RotRightMsg::New() {
    return new RotRightMsg;
}

// 0x0011d458
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *RotRightMsg::Clone() {
    return new RotRightMsg(*this);
}

// 0x0011d4a8
int RotRightMsg::Type() {
    return g_nRotRightMsgType;
}

// 0x0011d4b8
const char *RotRightMsg::Name() {
    return "RotRightMsg";
}
