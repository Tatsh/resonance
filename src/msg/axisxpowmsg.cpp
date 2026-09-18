#include "msg/axisxpowmsg.h"

// 0x003db060. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *AxisXPowMsg::Clone() {
    return new AxisXPowMsg(*this);
}

// 0x003db0b8
int AxisXPowMsg::Type() {
    return g_nAxisXPowMsgType;
}

// 0x003db0c8
const char *AxisXPowMsg::Name() {
    return "AxisXPowMsg";
}
