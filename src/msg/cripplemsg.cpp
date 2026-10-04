#include "msg/cripplemsg.h"

#include <iostream>

#include "game/player.h"

Message *CrippleMsg::New() {
    return new CrippleMsg;
}

Message *CrippleMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new CrippleMsg(*this);
}

int CrippleMsg::Type() {
    return sID;
}

const char *CrippleMsg::GetName() const {
    return "CrippleMsg";
}

void CrippleMsg::PrintExtra(std::ostream &stream) const {
    stream << "tr#" << mTrack;
    stream << " p#" << mPlayer->mPlayerId;
}
