#include "msg/barstatusmsg.h"

#include <bitset>
#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// 0x003d7440
Message *BarStatusMsg::New() {
    return new BarStatusMsg;
}

// 0x003d8670
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

// 0x003defd0
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *BarStatusMsg::Clone() {
    return new BarStatusMsg(*this);
}

// 0x003df050
int BarStatusMsg::Type() {
    return g_nBarStatusMsgType;
}

// 0x003df060
const char *BarStatusMsg::Name() {
    return "BarStatusMsg";
}

// 0x003df1f8
int BarStatusMsg::Has(int nField) {
    return (mFlags & nField) != 0;
}
