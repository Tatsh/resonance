#include "msg/barstatusmsg.h"

#include <bitset>
#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// NTSC-U/C: 0x003d7440, PAL: 0x0040f340
Message *BarStatusMsg::New() {
    return new BarStatusMsg;
}

// NTSC-U/C: 0x003d8670, PAL: 0x00410a68
// The colour name is copied into a temporary before it is written.
void BarStatusMsg::Print(std::ostream &stream) {
    stream << "b#" << mBar << " tr#" << mTrack;
    if (mFlags & kFieldPlayer) {
        stream << " " << HxStr(GetPlayer()->mColorName);
    }
    if (mFlags & kFieldEnabled) {
        if (GetEnabled()) {
            stream << " enabled";
        } else {
            stream << " disabled";
        }
    }
    if (mFlags & kFieldPowerup) {
        stream << " pow:" << GetPowerup();
    }
    if (mFlags & kFieldEffects) {
        stream << " effect:" << GetEffects();
    }
}

// NTSC-U/C: 0x003defd0, PAL: 0x00417428
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *BarStatusMsg::Clone() {
    return new BarStatusMsg(*this);
}

// NTSC-U/C: 0x003df050, PAL: 0x004174a8
int BarStatusMsg::Type() {
    return g_nBarStatusMsgType;
}

// NTSC-U/C: 0x003df060, PAL: 0x004174b8
const char *BarStatusMsg::Name() {
    return "BarStatusMsg";
}

// NTSC-U/C: 0x003df1f8, PAL: 0x00417650
int BarStatusMsg::Has(int nField) {
    return (mFlags & nField) != 0;
}
