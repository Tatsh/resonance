#include "msg/rotrightmsg.h"

Message *RotRightMsg::New() {
    return new RotRightMsg;
}

Message *RotRightMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new RotRightMsg(*this);
}

int RotRightMsg::Type() {
    return g_nRotRightMsgType;
}

const char *RotRightMsg::GetName() const {
    return "RotRightMsg";
}
