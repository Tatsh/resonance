#include "msg/stopriffmsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

// 0x003d6968
Message *StopRiffMsg::New() {
    return new StopRiffMsg;
}

// 0x003da7f8. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *StopRiffMsg::Clone() {
    return new StopRiffMsg(*this);
}

// 0x003da858
int StopRiffMsg::Type() {
    return g_nStopRiffMsgType;
}

// 0x003da868
const char *StopRiffMsg::Name() {
    return "StopRiffMsg";
}

// 0x003e3098. The colour name is copied into a temporary before it is written.
void StopRiffMsg::Print(std::ostream &stream) {
    mPosition.Print(stream);
    stream << " " << HxStr(mPlayer->mColorName) << " b#" << mUnknown04;
}
