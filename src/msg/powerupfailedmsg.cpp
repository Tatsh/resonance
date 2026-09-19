#include "msg/powerupfailedmsg.h"

// 0x003d7038
Message *PowerupFailedMsg::New() {
    return new PowerupFailedMsg;
}

// 0x001ca778. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *PowerupFailedMsg::Clone() {
    return new PowerupFailedMsg(*this);
}

// 0x001ca7c8
int PowerupFailedMsg::Type() {
    return g_nPowerupFailedMsgType;
}

// 0x001ca7d8
const char *PowerupFailedMsg::Name() {
    return "PowerupFailedMsg";
}
