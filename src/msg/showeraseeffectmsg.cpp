#include "msg/showeraseeffectmsg.h"

// 0x003d7210
Message *ShowEraseEffectMsg::New() {
    return new ShowEraseEffectMsg;
}

// 0x0019d6b0
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *ShowEraseEffectMsg::Clone() {
    return new ShowEraseEffectMsg(*this);
}

// 0x0019d718
int ShowEraseEffectMsg::Type() {
    return g_nShowEraseEffectMsgType;
}

// 0x0019d728
const char *ShowEraseEffectMsg::Name() {
    return "ShowEraseEffectMsg";
}
