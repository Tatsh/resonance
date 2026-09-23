#include "msg/multiplierstatemsg.h"

// 0x003d6c98
Message *MultiplierStateMsg::New() {
    return new MultiplierStateMsg;
}

// 0x00122610
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *MultiplierStateMsg::Clone() {
    return new MultiplierStateMsg(*this);
}

// 0x00122668
int MultiplierStateMsg::Type() {
    return g_nMultiplierStateMsgType;
}

// 0x00122678
const char *MultiplierStateMsg::Name() {
    return "MultiplierStateMsg";
}
