#include "msg/choosepowerupmsg.h"

#include <iostream>

// No address of its own. The two collections expand it into five call sites.
ChoosePowerupMsg::ChoosePowerupMsg(int nIndex, Player *pOwner, int nType)
    : mIndex(nIndex), mOwner(pOwner), mType(nType) {
}

// NTSC-U/C: 0x003d6f90, PAL: 0x0040ee80
Message *ChoosePowerupMsg::New() {
    return new ChoosePowerupMsg;
}

// NTSC-U/C: 0x003dd0b8, PAL: 0x004154f0
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *ChoosePowerupMsg::Clone() {
    return new ChoosePowerupMsg(*this);
}

// NTSC-U/C: 0x003dd110, PAL: 0x00415548
int ChoosePowerupMsg::Type() {
    return g_nChoosePowerupMsgType;
}

// NTSC-U/C: 0x003dd120, PAL: 0x00415558
const char *ChoosePowerupMsg::GetName() const {
    return "ChoosePowerupMsg";
}

// NTSC-U/C: 0x003e3ce0, PAL: 0x0041beb0
void ChoosePowerupMsg::PrintExtra(std::ostream &stream) const {
    stream << mIndex;
}
