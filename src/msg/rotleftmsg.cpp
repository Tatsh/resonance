#include "msg/rotleftmsg.h"

Message *RotLeftMsg::New() {
    return new RotLeftMsg;
}

Message *RotLeftMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new RotLeftMsg(*this);
}

int RotLeftMsg::Type() {
    return g_nRotLeftMsgType;
}

const char *RotLeftMsg::GetName() const {
    return "RotLeftMsg";
}
