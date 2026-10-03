#include "msg/cleargemsmsg.h"

// NTSC-U/C: 0x003d7480, PAL: 0x0040f380
Message *ClearGemsMsg::New() {
    return new ClearGemsMsg;
}

// NTSC-U/C: 0x0019d590, PAL: 0x001a32f8
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *ClearGemsMsg::Clone() {
    return new ClearGemsMsg(*this);
}

// NTSC-U/C: 0x0019d5e0, PAL: 0x001a3348
int ClearGemsMsg::Type() {
    return g_nClearGemsMsgType;
}

// NTSC-U/C: 0x0019d5f0, PAL: 0x001a3358
const char *ClearGemsMsg::Name() {
    return "ClearGemsMsg";
}
