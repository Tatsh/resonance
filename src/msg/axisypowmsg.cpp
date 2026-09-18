#include "msg/axisypowmsg.h"

// 0x003daea0. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *AxisYPowMsg::Clone() {
    return new AxisYPowMsg(*this);
}

// 0x003daef8
int AxisYPowMsg::Type() {
    return g_nAxisYPowMsgType;
}

// 0x003daf08
const char *AxisYPowMsg::Name() {
    return "AxisYPowMsg";
}
