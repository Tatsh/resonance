#include "msg/cleargemmsg.h"

// 0x003d74b8
Message *ClearGemMsg::New() {
    return new ClearGemMsg;
}

// 0x001bf8e0
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *ClearGemMsg::Clone() {
    return new ClearGemMsg(*this);
}

// 0x001bf938
int ClearGemMsg::Type() {
    return g_nClearGemMsgType;
}

// 0x001bf948
const char *ClearGemMsg::Name() {
    return "ClearGemMsg";
}
