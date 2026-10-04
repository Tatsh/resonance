#include "msg/phrasemuffedmsg.h"

#include <iostream>

#include "game/player.h"

Message *PhraseMuffedMsg::New() {
    return new PhraseMuffedMsg;
}

Message *PhraseMuffedMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new PhraseMuffedMsg(*this);
}

int PhraseMuffedMsg::Type() {
    return g_nPhraseMuffedMsgType;
}

const char *PhraseMuffedMsg::GetName() const {
    return "PhraseMuffedMsg";
}

void PhraseMuffedMsg::PrintExtra(std::ostream &stream) const {
    std::ostream &rest = stream << "tr#" << mTrack << " ";
    mPlayer->Print(rest);
    std::ostream &tail = rest << " ";
    mPosition.Print(tail);
    tail << " tried:" << mTried;
}
