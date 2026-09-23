#include "msg/phrasemuffedmsg.h"

#include <iostream>

#include "game/player.h"

// 0x003d76f0
Message *PhraseMuffedMsg::New() {
    return new PhraseMuffedMsg;
}

// 0x003e0038
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *PhraseMuffedMsg::Clone() {
    return new PhraseMuffedMsg(*this);
}

// 0x003e0098
int PhraseMuffedMsg::Type() {
    return g_nPhraseMuffedMsgType;
}

// 0x003e00a8
const char *PhraseMuffedMsg::Name() {
    return "PhraseMuffedMsg";
}

// 0x003e4350
void PhraseMuffedMsg::Print(std::ostream &stream) {
    std::ostream &rest = stream << "tr#" << mTrack << " ";
    mPlayer->Print(rest);
    std::ostream &tail = rest << " ";
    mPosition.Print(tail);
    tail << " tried:" << mTried;
}
