#include "msg/cleargemsmsg.h"

// 0x003d7480
Message *ClearGemsMsg::New() {
    return new ClearGemsMsg;
}

// 0x0019d590
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *ClearGemsMsg::Clone() {
    return new ClearGemsMsg(*this);
}

// 0x0019d5e0
int ClearGemsMsg::Type() {
    return g_nClearGemsMsgType;
}

// 0x0019d5f0
const char *ClearGemsMsg::Name() {
    return "ClearGemsMsg";
}
