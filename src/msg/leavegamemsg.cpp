#include "msg/leavegamemsg.h"

// 0x003e0f80. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *LeaveGameMsg::Clone() {
    return new LeaveGameMsg(*this);
}

// 0x003e0fb8
int LeaveGameMsg::Type() {
    return g_nLeaveGameMsgType;
}

// 0x003e0fc8
const char *LeaveGameMsg::Name() {
    return "LeaveGameMsg";
}
