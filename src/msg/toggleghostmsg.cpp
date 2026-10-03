#include "msg/toggleghostmsg.h"

// NTSC-U/C: 0x003d6c28, PAL: 0x0040eb18
Message *ToggleGhostMsg::New() {
    return new ToggleGhostMsg;
}

// NTSC-U/C: 0x0011d7c0, PAL: 0x0011dd48
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *ToggleGhostMsg::Clone() {
    return new ToggleGhostMsg(*this);
}

// NTSC-U/C: 0x0011d810, PAL: 0x0011dd98
int ToggleGhostMsg::Type() {
    return g_nToggleGhostMsgType;
}

// NTSC-U/C: 0x0011d820, PAL: 0x0011dda8
const char *ToggleGhostMsg::Name() {
    return "ToggleGhostMsg";
}
