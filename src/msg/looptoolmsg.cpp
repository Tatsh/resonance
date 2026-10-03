#include "msg/looptoolmsg.h"

// NTSC-U/C: 0x003d6be8, PAL: 0x0040ead8
Message *LoopToolMsg::New() {
    return new LoopToolMsg;
}

// NTSC-U/C: 0x0011d8e0, PAL: 0x0011de68
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *LoopToolMsg::Clone() {
    return new LoopToolMsg(*this);
}

// NTSC-U/C: 0x0011d938, PAL: 0x0011dec0
int LoopToolMsg::Type() {
    return g_nLoopToolMsgType;
}

// NTSC-U/C: 0x0011d948, PAL: 0x0011ded0
const char *LoopToolMsg::GetName() const {
    return "LoopToolMsg";
}
