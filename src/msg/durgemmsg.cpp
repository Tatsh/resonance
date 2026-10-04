#include "msg/durgemmsg.h"

Message *DurGemMsg::New() {
    return new DurGemMsg;
}

Message *DurGemMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new DurGemMsg(*this);
}

int DurGemMsg::Type() {
    return g_nDurGemMsgType;
}

const char *DurGemMsg::GetName() const {
    return "DurGemMsg";
}
