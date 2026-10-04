#include "msg/barstatusmsg.h"

#include <bitset>
#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

Message *BarStatusMsg::New() {
    return new BarStatusMsg;
}

void BarStatusMsg::PrintExtra(std::ostream &stream) const {
    stream << "b#" << mBar << " tr#" << mTrack;
    if (mFlags & kFieldPlayer) {
        // The colour name is copied into a temporary before it is written.
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

Message *BarStatusMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new BarStatusMsg(*this);
}

int BarStatusMsg::Type() {
    return g_nBarStatusMsgType;
}

const char *BarStatusMsg::GetName() const {
    return "BarStatusMsg";
}

int BarStatusMsg::Has(int nField) const {
    return (mFlags & nField) != 0;
}
