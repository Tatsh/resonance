#include "msg/choosepowerupmsg.h"

#include <iostream>

ChoosePowerupMsg::ChoosePowerupMsg(int nIndex, Player *pOwner, int nType)
    : mIndex(nIndex), mOwner(pOwner), mType(nType) {
}

Message *ChoosePowerupMsg::New() {
    return new ChoosePowerupMsg;
}

Message *ChoosePowerupMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new ChoosePowerupMsg(*this);
}

int ChoosePowerupMsg::Type() {
    return g_nChoosePowerupMsgType;
}

const char *ChoosePowerupMsg::GetName() const {
    return "ChoosePowerupMsg";
}

void ChoosePowerupMsg::PrintExtra(std::ostream &stream) const {
    stream << mIndex;
}
