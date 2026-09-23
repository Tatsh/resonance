#include "msg/seekermsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

namespace {

constexpr int kTicksPerBar = 1920;

} // namespace

// 0x003d6f10
Message *SeekerMsg::New() {
    return new SeekerMsg;
}

// 0x003dcce0
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *SeekerMsg::Clone() {
    return new SeekerMsg(*this);
}

// 0x003dcd50
int SeekerMsg::Type() {
    return g_nSeekerMsgType;
}

// 0x003dcd60
const char *SeekerMsg::Name() {
    return "SeekerMsg";
}

// 0x003d81f0
// The colour name is copied into a temporary before it is written, and the discarded
// IsFiniteMBT() call is the shape of an assertion compiled without its report.
void SeekerMsg::Print(std::ostream &stream) {
    stream << HxStr(mPlayer->mColorName);
    if (mEnabled != 0) {
        std::ostream &rest = stream << " bars[" << mFirstBar << " - " << mFirstBar + mBarCount
                                    << "]" << " tr#" << mTrack << " when: ";
        IsFiniteMBT(kTicksPerBar);
        rest << mWhen.mTick / kTicksPerBar;
    } else {
        stream << " (off)";
    }
}
