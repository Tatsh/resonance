#include "msg/looptoolmsg.h"

// 0x003d6be8
Message *LoopToolMsg::New() {
    return new LoopToolMsg;
}

// 0x0011d8e0. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *LoopToolMsg::Clone() {
    return new LoopToolMsg(*this);
}

// 0x0011d938
int LoopToolMsg::Type() {
    return g_nLoopToolMsgType;
}

// 0x0011d948
const char *LoopToolMsg::Name() {
    return "LoopToolMsg";
}
