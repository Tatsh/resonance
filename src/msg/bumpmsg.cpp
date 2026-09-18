#include "msg/bumpmsg.h"

// 0x003e13d0. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *BumpMsg::Clone() {
    return new BumpMsg(*this);
}

// 0x003e1440
int BumpMsg::Type() {
    return g_nBumpMsgType;
}

// 0x003e1450
const char *BumpMsg::Name() {
    return "BumpMsg";
}
