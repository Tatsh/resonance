#include "msg/remixfxmsg.h"

Message *RemixFXMsg::New() {
    return new RemixFXMsg;
}

Message *RemixFXMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new RemixFXMsg(*this);
}

int RemixFXMsg::Type() {
    return g_nRemixFXMsgType;
}

const char *RemixFXMsg::GetName() const {
    return "RemixFXMsg";
}
