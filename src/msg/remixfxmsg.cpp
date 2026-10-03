#include "msg/remixfxmsg.h"

// NTSC-U/C: 0x003d7070, PAL: 0x0040ef60
Message *RemixFXMsg::New() {
    return new RemixFXMsg;
}

// NTSC-U/C: 0x001a6250, PAL: 0x001abfb8
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *RemixFXMsg::Clone() {
    return new RemixFXMsg(*this);
}

// NTSC-U/C: 0x001a62b8, PAL: 0x001ac020
int RemixFXMsg::Type() {
    return g_nRemixFXMsgType;
}

// NTSC-U/C: 0x001a62c8, PAL: 0x001ac030
const char *RemixFXMsg::GetName() const {
    return "RemixFXMsg";
}
