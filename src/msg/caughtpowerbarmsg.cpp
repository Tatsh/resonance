#include "msg/caughtpowerbarmsg.h"

// 0x003dcf10. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *CaughtPowerbarMsg::Clone() {
    return new CaughtPowerbarMsg(*this);
}

// 0x003dcf60
int CaughtPowerbarMsg::Type() {
    return g_nCaughtPowerbarMsgType;
}

// 0x003dcf70
const char *CaughtPowerbarMsg::Name() {
    return "CaughtPowerbarMsg";
}
