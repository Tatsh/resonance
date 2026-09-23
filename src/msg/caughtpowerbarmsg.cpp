#include "msg/caughtpowerbarmsg.h"

#include <iostream>

// 0x003d6f58
Message *CaughtPowerbarMsg::New() {
    return new CaughtPowerbarMsg;
}

// 0x003dcf10
// The field copies are the compiler expanding the implicit copy
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

// 0x003e3cb8
void CaughtPowerbarMsg::Print(std::ostream &stream) {
    stream << mKind;
}
