#include "msg/seekermsg.h"

// 0x003dcce0. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *SeekerMsg::Clone() {
    return new SeekerMsg(*this);
}

// 0x003dcd50
int SeekerMsg::Type() {
    return g_nSeekerMsgType;
}

// 0x003dcd60
const char *SeekerMsg::Name() {
    return "SeekerMsg";
}
