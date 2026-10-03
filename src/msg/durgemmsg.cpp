#include "msg/durgemmsg.h"

// NTSC-U/C: 0x003d7538, PAL: 0x0040f438
Message *DurGemMsg::New() {
    return new DurGemMsg;
}

// NTSC-U/C: 0x001a4388, PAL: 0x001aa0f0
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *DurGemMsg::Clone() {
    return new DurGemMsg(*this);
}

// NTSC-U/C: 0x001a4400, PAL: 0x001aa168
int DurGemMsg::Type() {
    return g_nDurGemMsgType;
}

// NTSC-U/C: 0x001a4410, PAL: 0x001aa178
const char *DurGemMsg::GetName() const {
    return "DurGemMsg";
}
