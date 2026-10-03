#include "msg/freestylefxmsg.h"

// NTSC-U/C: 0x003d7398, PAL: 0x0040f298
Message *FreestyleFXMsg::New() {
    return new FreestyleFXMsg;
}

// NTSC-U/C: 0x00116500, PAL: 0x001169a8
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *FreestyleFXMsg::Clone() {
    return new FreestyleFXMsg(*this);
}

// NTSC-U/C: 0x00116558, PAL: 0x00116a00
int FreestyleFXMsg::Type() {
    return g_nFreestyleFXMsgType;
}

// NTSC-U/C: 0x00116568, PAL: 0x00116a10
const char *FreestyleFXMsg::GetName() const {
    return "FreestyleFXMsg";
}
