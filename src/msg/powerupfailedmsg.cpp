#include "msg/powerupfailedmsg.h"

// NTSC-U/C: 0x003d7038, PAL: 0x0040ef28
Message *PowerupFailedMsg::New() {
    return new PowerupFailedMsg;
}

// NTSC-U/C: 0x001ca778, PAL: 0x001d0630
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *PowerupFailedMsg::Clone() {
    return new PowerupFailedMsg(*this);
}

// NTSC-U/C: 0x001ca7c8, PAL: 0x001d0680
int PowerupFailedMsg::Type() {
    return g_nPowerupFailedMsgType;
}

// NTSC-U/C: 0x001ca7d8, PAL: 0x001d0690
const char *PowerupFailedMsg::Name() {
    return "PowerupFailedMsg";
}
