#include "msg/multipliermsg.h"

// NTSC-U/C: 0x003d6c60, PAL: 0x0040eb50
Message *MultiplierMsg::New() {
    return new MultiplierMsg;
}

// NTSC-U/C: 0x001cab30, PAL: 0x001d09e8
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *MultiplierMsg::Clone() {
    return new MultiplierMsg(*this);
}

// NTSC-U/C: 0x001cab88, PAL: 0x001d0a40
int MultiplierMsg::Type() {
    return g_nMultiplierMsgType;
}

// NTSC-U/C: 0x001cab98, PAL: 0x001d0a50
const char *MultiplierMsg::Name() {
    return "MultiplierMsg";
}
