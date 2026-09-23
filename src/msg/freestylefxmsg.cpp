#include "msg/freestylefxmsg.h"

// 0x003d7398
Message *FreestyleFXMsg::New() {
    return new FreestyleFXMsg;
}

// 0x00116500
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *FreestyleFXMsg::Clone() {
    return new FreestyleFXMsg(*this);
}

// 0x00116558
int FreestyleFXMsg::Type() {
    return g_nFreestyleFXMsgType;
}

// 0x00116568
const char *FreestyleFXMsg::Name() {
    return "FreestyleFXMsg";
}
