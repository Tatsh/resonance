#include "msg/axebuttonmsg.h"

// NTSC-U/C: 0x003d76b8, PAL: 0x0040f5b8
Message *AxeButtonMsg::New() {
    return new AxeButtonMsg;
}

// NTSC-U/C: 0x0019a748, PAL: 0x001a04b0
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *AxeButtonMsg::Clone() {
    return new AxeButtonMsg(*this);
}

// NTSC-U/C: 0x0019a7a0, PAL: 0x001a0508
int AxeButtonMsg::Type() {
    return g_nAxeButtonMsgType;
}

// NTSC-U/C: 0x0019a7b0, PAL: 0x001a0518
const char *AxeButtonMsg::GetName() const {
    return "AxeButtonMsg";
}
