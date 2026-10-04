#include "msg/powerupcountmsg.h"

#include <iostream>

Message *PowerupCountMsg::New() {
    return new PowerupCountMsg;
}

PowerupCountMsg::PowerupCountMsg(int nIndex, int nCount, Player *pOwner)
    : mIndex(nIndex), mCount(nCount), mOwner(pOwner) {
}

Message *PowerupCountMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new PowerupCountMsg(*this);
}

int PowerupCountMsg::Type() {
    return g_nPowerupCountMsgType;
}

const char *PowerupCountMsg::GetName() const {
    return "PowerupCountMsg";
}

void PowerupCountMsg::PrintExtra(std::ostream &stream) const {
    stream << mIndex << "/" << mCount;
}
