#include "msg/gemmsg.h"

// 0x003df540. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *GemMsg::Clone() {
    return new GemMsg(*this);
}

// 0x003df5a8
int GemMsg::Type() {
    return g_nGemMsgType;
}

// 0x003df5b8
const char *GemMsg::Name() {
    return "GemMsg";
}
