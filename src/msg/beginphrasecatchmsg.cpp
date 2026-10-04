#include "msg/beginphrasecatchmsg.h"

Message *BeginPhraseCatchMsg::New() {
    return new BeginPhraseCatchMsg;
}

Message *BeginPhraseCatchMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new BeginPhraseCatchMsg(*this);
}

int BeginPhraseCatchMsg::Type() {
    return g_nBeginPhraseCatchMsgType;
}

const char *BeginPhraseCatchMsg::GetName() const {
    return "BeginPhraseCatchMsg";
}
