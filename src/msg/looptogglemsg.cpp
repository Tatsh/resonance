#include "msg/looptogglemsg.h"

Message *LoopToggleMsg::New() {
    return new LoopToggleMsg;
}

Message *LoopToggleMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new LoopToggleMsg(*this);
}

int LoopToggleMsg::Type() {
    return g_nLoopToggleMsgType;
}

const char *LoopToggleMsg::GetName() const {
    return "LoopToggleMsg";
}
