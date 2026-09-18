#include "msg/pointamountmsg.h"

// 0x003e0a48. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *PointAmountMsg::Clone() {
    return new PointAmountMsg(*this);
}

// 0x003e0a98
int PointAmountMsg::Type() {
    return g_nPointAmountMsgType;
}

// 0x003e0aa8
const char *PointAmountMsg::Name() {
    return "PointAmountMsg";
}
