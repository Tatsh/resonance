#include "msg/leavegamemsg.h"

// NTSC-U/C: 0x003d7940, PAL: 0x0040f840
Message *LeaveGameMsg::New() {
    return new LeaveGameMsg;
}

// NTSC-U/C: 0x003e0f80, PAL: 0x004193d8
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *LeaveGameMsg::Clone() {
    return new LeaveGameMsg(*this);
}

// NTSC-U/C: 0x003e0fb8, PAL: 0x00419410
int LeaveGameMsg::Type() {
    return g_nLeaveGameMsgType;
}

// NTSC-U/C: 0x003e0fc8, PAL: 0x00419420
const char *LeaveGameMsg::GetName() const {
    return "LeaveGameMsg";
}
