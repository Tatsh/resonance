#include "msg/refreshnetmsg.h"

#include <iostream>

// 0x003d72f0
Message *RefreshNetMsg::New() {
    return new RefreshNetMsg;
}

// 0x003de630
// The field copies are the compiler expanding the implicit copy
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

// 0x003e3e08
void RefreshNetMsg::Print(std::ostream &stream) {
    stream << "tr#" << mUnknown0c << " bars " << mUnknown04 << " - " << mUnknown08;
}
