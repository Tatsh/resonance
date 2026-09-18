#include "msg/remotetrackselectmsg.h"

// 0x003dcb08. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *RemoteTrackSelectMsg::Clone() {
    return new RemoteTrackSelectMsg(*this);
}

// 0x003dcb68
int RemoteTrackSelectMsg::Type() {
    return g_nRemoteTrackSelectMsgType;
}

// 0x003dcb78
const char *RemoteTrackSelectMsg::Name() {
    return "RemoteTrackSelectMsg";
}
