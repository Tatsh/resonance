#include "msg/rotleftmsg.h"

// NTSC-U/C: 0x003d68a8, PAL: 0x0040e798
Message *RotLeftMsg::New() {
    return new RotLeftMsg;
}

// NTSC-U/C: 0x0011d338, PAL: 0x0011d8c0
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *RotLeftMsg::Clone() {
    return new RotLeftMsg(*this);
}

// NTSC-U/C: 0x0011d388, PAL: 0x0011d910
int RotLeftMsg::Type() {
    return g_nRotLeftMsgType;
}

// NTSC-U/C: 0x0011d398, PAL: 0x0011d920
const char *RotLeftMsg::GetName() const {
    return "RotLeftMsg";
}
