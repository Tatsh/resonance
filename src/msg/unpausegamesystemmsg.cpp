#include "msg/unpausegamesystemmsg.h"

// NTSC-U/C: 0x003d7bf8, PAL: 0x0040fb10
Message *UnpauseGameSystemMsg::New() {
    return new UnpauseGameSystemMsg;
}

// NTSC-U/C: 0x00311c68, PAL: 0x003379c8
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *UnpauseGameSystemMsg::Clone() {
    return new UnpauseGameSystemMsg(*this);
}

// NTSC-U/C: 0x00311ca0, PAL: 0x00337a00
int UnpauseGameSystemMsg::Type() {
    return g_nUnpauseGameSystemMsgType;
}

// NTSC-U/C: 0x00311cb0, PAL: 0x00337a10
const char *UnpauseGameSystemMsg::GetName() const {
    return "UnpauseGameSystemMsg";
}
