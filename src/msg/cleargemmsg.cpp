#include "msg/cleargemmsg.h"

Message *ClearGemMsg::New() {
    return new ClearGemMsg;
}

Message *ClearGemMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new ClearGemMsg(*this);
}

int ClearGemMsg::Type() {
    return g_nClearGemMsgType;
}

const char *ClearGemMsg::GetName() const {
    return "ClearGemMsg";
}
