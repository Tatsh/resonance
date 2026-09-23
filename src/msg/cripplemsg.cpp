#include "msg/cripplemsg.h"

#include <iostream>

#include "game/player.h"

// 0x003d7ca0
Message *CrippleMsg::New() {
    return new CrippleMsg;
}

// 0x003e2788. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *CrippleMsg::Clone() {
    return new CrippleMsg(*this);
}

// 0x003e27f8
int CrippleMsg::Type() {
    return g_nCrippleMsgType;
}

// 0x003e2808
const char *CrippleMsg::Name() {
    return "CrippleMsg";
}

// 0x003e4290
void CrippleMsg::Print(std::ostream &stream) {
    stream << "tr#" << mTrack;
    stream << " p#" << mPlayer->mId20;
}
