#include "msg/cleargemsmsg.h"

Message *ClearGemsMsg::New() {
    return new ClearGemsMsg;
}

Message *ClearGemsMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new ClearGemsMsg(*this);
}

int ClearGemsMsg::Type() {
    return g_nClearGemsMsgType;
}

const char *ClearGemsMsg::GetName() const {
    return "ClearGemsMsg";
}
