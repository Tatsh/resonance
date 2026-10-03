#include "msg/powerupcountmsg.h"

#include <iostream>

// NTSC-U/C: 0x003d6fc8, PAL: 0x0040eeb8
Message *PowerupCountMsg::New() {
    return new PowerupCountMsg;
}

// No address of its own. PowerupCollection expands it into three call sites.
PowerupCountMsg::PowerupCountMsg(int nIndex, int nCount, Player *pOwner)
    : mIndex(nIndex), mCount(nCount), mOwner(pOwner) {
}

// NTSC-U/C: 0x003dd270, PAL: 0x004156a8
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *PowerupCountMsg::Clone() {
    return new PowerupCountMsg(*this);
}

// NTSC-U/C: 0x003dd2c8, PAL: 0x00415700
int PowerupCountMsg::Type() {
    return g_nPowerupCountMsgType;
}

// NTSC-U/C: 0x003dd2d8, PAL: 0x00415710
const char *PowerupCountMsg::GetName() const {
    return "PowerupCountMsg";
}

// NTSC-U/C: 0x003e3d08, PAL: 0x0041bed8
void PowerupCountMsg::PrintExtra(std::ostream &stream) const {
    stream << mIndex << "/" << mCount;
}
