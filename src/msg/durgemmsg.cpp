#include "msg/durgemmsg.h"

// 0x003d7538
Message *DurGemMsg::New() {
    return new DurGemMsg;
}

// 0x001a4388
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *DurGemMsg::Clone() {
    return new DurGemMsg(*this);
}

// 0x001a4400
int DurGemMsg::Type() {
    return g_nDurGemMsgType;
}

// 0x001a4410
const char *DurGemMsg::Name() {
    return "DurGemMsg";
}
