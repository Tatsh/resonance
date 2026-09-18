#include "msg/eraseoffmsg.h"

// 0x003db5b8. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *EraseOffMsg::Clone() {
    return new EraseOffMsg(*this);
}

// 0x003db610
int EraseOffMsg::Type() {
    return g_nEraseOffMsgType;
}

// 0x003db620
const char *EraseOffMsg::Name() {
    return "EraseOffMsg";
}
