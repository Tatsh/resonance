#include "msg/multiplierstatemsg.h"

// NTSC-U/C: 0x003d6c98, PAL: 0x0040eb88
Message *MultiplierStateMsg::New() {
    return new MultiplierStateMsg;
}

// NTSC-U/C: 0x00122610, PAL: 0x00122c28
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *MultiplierStateMsg::Clone() {
    return new MultiplierStateMsg(*this);
}

// NTSC-U/C: 0x00122668, PAL: 0x00122c80
int MultiplierStateMsg::Type() {
    return g_nMultiplierStateMsgType;
}

// NTSC-U/C: 0x00122678, PAL: 0x00122c90
const char *MultiplierStateMsg::Name() {
    return "MultiplierStateMsg";
}
