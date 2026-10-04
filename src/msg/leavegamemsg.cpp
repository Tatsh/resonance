#include "msg/leavegamemsg.h"

Message *LeaveGameMsg::New() {
    return new LeaveGameMsg;
}

Message *LeaveGameMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new LeaveGameMsg(*this);
}

int LeaveGameMsg::Type() {
    return g_nLeaveGameMsgType;
}

const char *LeaveGameMsg::GetName() const {
    return "LeaveGameMsg";
}
