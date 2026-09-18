#include "msg/erasemsg.h"

// 0x003db3e0. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *EraseMsg::Clone() {
    return new EraseMsg(*this);
}

// 0x003db440
int EraseMsg::Type() {
    return g_nEraseMsgType;
}

// 0x003db450
const char *EraseMsg::Name() {
    return "EraseMsg";
}
