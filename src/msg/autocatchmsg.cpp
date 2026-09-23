#include "msg/autocatchmsg.h"

#include <iostream>

#include "game/player.h"

// 0x003d7c68
Message *AutoCatchMsg::New() {
    return new AutoCatchMsg;
}

// 0x003e2580. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *AutoCatchMsg::Clone() {
    return new AutoCatchMsg(*this);
}

// 0x003e25f0
int AutoCatchMsg::Type() {
    return g_nAutoCatchMsgType;
}

// 0x003e2600
const char *AutoCatchMsg::Name() {
    return "AutoCatchMsg";
}

// 0x003e4208
void AutoCatchMsg::Print(std::ostream &stream) {
    stream << "tr#" << mTrack;
    stream << " p#" << mPlayer->mId20 << " " << mBar;
}
