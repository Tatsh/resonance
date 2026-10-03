#include "msg/cleargemmsg.h"

// NTSC-U/C: 0x003d74b8, PAL: 0x0040f3b8
Message *ClearGemMsg::New() {
    return new ClearGemMsg;
}

// NTSC-U/C: 0x001bf8e0, PAL: 0x001c5700
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *ClearGemMsg::Clone() {
    return new ClearGemMsg(*this);
}

// NTSC-U/C: 0x001bf938, PAL: 0x001c5758
int ClearGemMsg::Type() {
    return g_nClearGemMsgType;
}

// NTSC-U/C: 0x001bf948, PAL: 0x001c5768
const char *ClearGemMsg::GetName() const {
    return "ClearGemMsg";
}
