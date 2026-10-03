#include "msg/caughtbarmsg.h"

// NTSC-U/C: 0x003d6d08, PAL: 0x0040ebf8
Message *CaughtBarMsg::New() {
    return new CaughtBarMsg;
}

// NTSC-U/C: 0x001b11b0, PAL: 0x001b6f60
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *CaughtBarMsg::Clone() {
    return new CaughtBarMsg(*this);
}

// NTSC-U/C: 0x001b1200, PAL: 0x001b6fb0
int CaughtBarMsg::Type() {
    return g_nCaughtBarMsgType;
}

// NTSC-U/C: 0x001b1210, PAL: 0x001b6fc0
const char *CaughtBarMsg::GetName() const {
    return "CaughtBarMsg";
}
