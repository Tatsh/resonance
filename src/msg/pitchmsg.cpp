#include "msg/pitchmsg.h"

// 0x003d7600
Message *PitchMsg::New() {
    return new PitchMsg;
}

// 0x001b3940. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *PitchMsg::Clone() {
    return new PitchMsg(*this);
}

// 0x001b39a0
int PitchMsg::Type() {
    return g_nPitchMsgType;
}

// 0x001b39b0
const char *PitchMsg::Name() {
    return "PitchMsg";
}
