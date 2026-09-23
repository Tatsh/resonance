#include "msg/choosepowerupmsg.h"

#include <iostream>

// No address of its own. The two collections expand it into five call sites.
ChoosePowerupMsg::ChoosePowerupMsg(int nIndex, Player *pOwner, int nType)
    : mIndex(nIndex), mOwner(pOwner), mType(nType) {
}

// 0x003d6f90
Message *ChoosePowerupMsg::New() {
    return new ChoosePowerupMsg;
}

// 0x003dd0b8. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *ChoosePowerupMsg::Clone() {
    return new ChoosePowerupMsg(*this);
}

// 0x003dd110
int ChoosePowerupMsg::Type() {
    return g_nChoosePowerupMsgType;
}

// 0x003dd120
const char *ChoosePowerupMsg::Name() {
    return "ChoosePowerupMsg";
}

// 0x003e3ce0
void ChoosePowerupMsg::Print(std::ostream &stream) {
    stream << mIndex;
}
