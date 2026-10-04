#include "msg/looptoolmsg.h"

Message *LoopToolMsg::New() {
    return new LoopToolMsg;
}

Message *LoopToolMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new LoopToolMsg(*this);
}

int LoopToolMsg::Type() {
    return g_nLoopToolMsgType;
}

const char *LoopToolMsg::GetName() const {
    return "LoopToolMsg";
}
