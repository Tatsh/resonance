#include "msg/powerupfailedmsg.h"

Message *PowerupFailedMsg::New() {
    return new PowerupFailedMsg;
}

Message *PowerupFailedMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new PowerupFailedMsg(*this);
}

int PowerupFailedMsg::Type() {
    return g_nPowerupFailedMsgType;
}

const char *PowerupFailedMsg::GetName() const {
    return "PowerupFailedMsg";
}
