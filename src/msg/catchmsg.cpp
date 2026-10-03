#include "msg/catchmsg.h"

// NTSC-U/C: 0x003d75c0, PAL: 0x0040f4c0
Message *CatchMsg::New() {
    return new CatchMsg;
}

// NTSC-U/C: 0x001b1068, PAL: 0x001b6e18
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *CatchMsg::Clone() {
    return new CatchMsg(*this);
}

// NTSC-U/C: 0x001b10e0, PAL: 0x001b6e90
int CatchMsg::Type() {
    return g_nCatchMsgType;
}

// NTSC-U/C: 0x001b10f0, PAL: 0x001b6ea0
const char *CatchMsg::Name() {
    return "CatchMsg";
}
