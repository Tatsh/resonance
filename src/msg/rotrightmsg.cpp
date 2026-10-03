#include "msg/rotrightmsg.h"

// NTSC-U/C: 0x003d68e8, PAL: 0x0040e7d8
Message *RotRightMsg::New() {
    return new RotRightMsg;
}

// NTSC-U/C: 0x0011d458, PAL: 0x0011d9e0
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *RotRightMsg::Clone() {
    return new RotRightMsg(*this);
}

// NTSC-U/C: 0x0011d4a8, PAL: 0x0011da30
int RotRightMsg::Type() {
    return g_nRotRightMsgType;
}

// NTSC-U/C: 0x0011d4b8, PAL: 0x0011da40
const char *RotRightMsg::GetName() const {
    return "RotRightMsg";
}
