#include "msg/multipliermsg.h"

// 0x003d6c60
Message *MultiplierMsg::New() {
    return new MultiplierMsg;
}

// 0x001cab30
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *MultiplierMsg::Clone() {
    return new MultiplierMsg(*this);
}

// 0x001cab88
int MultiplierMsg::Type() {
    return g_nMultiplierMsgType;
}

// 0x001cab98
const char *MultiplierMsg::Name() {
    return "MultiplierMsg";
}
