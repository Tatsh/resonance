#include "msg/showeraseeffectmsg.h"

// NTSC-U/C: 0x003d7210, PAL: 0x0040f110
Message *ShowEraseEffectMsg::New() {
    return new ShowEraseEffectMsg;
}

// NTSC-U/C: 0x0019d6b0, PAL: 0x001a3418
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *ShowEraseEffectMsg::Clone() {
    return new ShowEraseEffectMsg(*this);
}

// NTSC-U/C: 0x0019d718, PAL: 0x001a3480
int ShowEraseEffectMsg::Type() {
    return g_nShowEraseEffectMsgType;
}

// NTSC-U/C: 0x0019d728, PAL: 0x001a3490
const char *ShowEraseEffectMsg::Name() {
    return "ShowEraseEffectMsg";
}
