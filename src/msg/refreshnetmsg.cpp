#include "msg/refreshnetmsg.h"

// 0x003de630. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *RefreshNetMsg::Clone() {
    return new RefreshNetMsg(*this);
}

// 0x003de688
int RefreshNetMsg::Type() {
    return g_nRefreshNetMsgType;
}

// 0x003de698
const char *RefreshNetMsg::Name() {
    return "RefreshNetMsg";
}
