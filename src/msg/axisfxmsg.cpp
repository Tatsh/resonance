#include "msg/axisfxmsg.h"

// 0x003dacc8. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *AxisFXMsg::Clone() {
    return new AxisFXMsg(*this);
}

// 0x003dad28
int AxisFXMsg::Type() {
    return g_nAxisFXMsgType;
}

// 0x003dad38
const char *AxisFXMsg::Name() {
    return "AxisFXMsg";
}
