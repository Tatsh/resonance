#include "msg/caughtpowerbarmsg.h"

#include <iostream>

// NTSC-U/C: 0x003d6f58, PAL: 0x0040ee48
Message *CaughtPowerbarMsg::New() {
    return new CaughtPowerbarMsg;
}

// NTSC-U/C: 0x003dcf10, PAL: 0x00415348
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *CaughtPowerbarMsg::Clone() {
    return new CaughtPowerbarMsg(*this);
}

// NTSC-U/C: 0x003dcf60, PAL: 0x00415398
int CaughtPowerbarMsg::Type() {
    return g_nCaughtPowerbarMsgType;
}

// NTSC-U/C: 0x003dcf70, PAL: 0x004153a8
const char *CaughtPowerbarMsg::Name() {
    return "CaughtPowerbarMsg";
}

// NTSC-U/C: 0x003e3cb8, PAL: 0x0041be88
void CaughtPowerbarMsg::Print(std::ostream &stream) {
    stream << mKind;
}
