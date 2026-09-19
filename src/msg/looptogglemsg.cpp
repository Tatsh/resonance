#include "msg/looptogglemsg.h"

// 0x003d7198
Message *LoopToggleMsg::New() {
    return new LoopToggleMsg;
}

// 0x001224f0. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *LoopToggleMsg::Clone() {
    return new LoopToggleMsg(*this);
}

// 0x00122540
int LoopToggleMsg::Type() {
    return g_nLoopToggleMsgType;
}

// 0x00122550
const char *LoopToggleMsg::Name() {
    return "LoopToggleMsg";
}
