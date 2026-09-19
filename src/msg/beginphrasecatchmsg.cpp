#include "msg/beginphrasecatchmsg.h"

// 0x003d7328
Message *BeginPhraseCatchMsg::New() {
    return new BeginPhraseCatchMsg;
}

// 0x0019d7e8. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *BeginPhraseCatchMsg::Clone() {
    return new BeginPhraseCatchMsg(*this);
}

// 0x0019d840
int BeginPhraseCatchMsg::Type() {
    return g_nBeginPhraseCatchMsgType;
}

// 0x0019d850
const char *BeginPhraseCatchMsg::Name() {
    return "BeginPhraseCatchMsg";
}
