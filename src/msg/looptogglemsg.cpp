#include "msg/looptogglemsg.h"

// NTSC-U/C: 0x003d7198, PAL: 0x0040f098
Message *LoopToggleMsg::New() {
    return new LoopToggleMsg;
}

// NTSC-U/C: 0x001224f0, PAL: 0x00122b08
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *LoopToggleMsg::Clone() {
    return new LoopToggleMsg(*this);
}

// NTSC-U/C: 0x00122540, PAL: 0x00122b58
int LoopToggleMsg::Type() {
    return g_nLoopToggleMsgType;
}

// NTSC-U/C: 0x00122550, PAL: 0x00122b68
const char *LoopToggleMsg::Name() {
    return "LoopToggleMsg";
}
