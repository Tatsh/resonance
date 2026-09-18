#include "msg/displaypointermsg.h"

// 0x003dd980. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *DisplayPointerMsg::Clone() {
    return new DisplayPointerMsg(*this);
}

// 0x003dd9d8
int DisplayPointerMsg::Type() {
    return g_nDisplayPointerMsgType;
}

// 0x003dd9e8
const char *DisplayPointerMsg::Name() {
    return "DisplayPointerMsg";
}
