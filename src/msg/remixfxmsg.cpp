#include "msg/remixfxmsg.h"

// 0x003d7070
Message *RemixFXMsg::New() {
    return new RemixFXMsg;
}

// 0x001a6250
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *RemixFXMsg::Clone() {
    return new RemixFXMsg(*this);
}

// 0x001a62b8
int RemixFXMsg::Type() {
    return g_nRemixFXMsgType;
}

// 0x001a62c8
const char *RemixFXMsg::Name() {
    return "RemixFXMsg";
}
