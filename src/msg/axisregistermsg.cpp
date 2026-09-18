#include "msg/axisregistermsg.h"

// 0x003daaf0. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *AxisRegisterMsg::Clone() {
    return new AxisRegisterMsg(*this);
}

// 0x003dab50
int AxisRegisterMsg::Type() {
    return g_nAxisRegisterMsgType;
}

// 0x003dab60
const char *AxisRegisterMsg::Name() {
    return "AxisRegisterMsg";
}
