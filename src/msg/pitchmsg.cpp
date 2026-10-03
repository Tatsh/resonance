#include "msg/pitchmsg.h"

// NTSC-U/C: 0x003d7600, PAL: 0x0040f500
Message *PitchMsg::New() {
    return new PitchMsg;
}

// NTSC-U/C: 0x001b3940, PAL: 0x001b9718
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *PitchMsg::Clone() {
    return new PitchMsg(*this);
}

// NTSC-U/C: 0x001b39a0, PAL: 0x001b9778
int PitchMsg::Type() {
    return g_nPitchMsgType;
}

// NTSC-U/C: 0x001b39b0, PAL: 0x001b9788
const char *PitchMsg::GetName() const {
    return "PitchMsg";
}
