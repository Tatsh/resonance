#include "msg/caughtpowerbarmsg.h"

#include <iostream>

Message *CaughtPowerbarMsg::New() {
    return new CaughtPowerbarMsg;
}

Message *CaughtPowerbarMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new CaughtPowerbarMsg(*this);
}

int CaughtPowerbarMsg::Type() {
    return g_nCaughtPowerbarMsgType;
}

const char *CaughtPowerbarMsg::GetName() const {
    return "CaughtPowerbarMsg";
}

void CaughtPowerbarMsg::PrintExtra(std::ostream &stream) const {
    stream << mKind;
}
