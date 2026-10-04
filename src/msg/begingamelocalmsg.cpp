#include "msg/begingamelocalmsg.h"

Message *BeginGameLocalMsg::New() {
    return new BeginGameLocalMsg;
}

Message *BeginGameLocalMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new BeginGameLocalMsg(*this);
}

int BeginGameLocalMsg::Type() {
    return g_nBeginGameLocalMsgType;
}

const char *BeginGameLocalMsg::GetName() const {
    return "BeginGameLocalMsg";
}
