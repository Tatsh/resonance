#include "msg/seekermsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

namespace {

constexpr int kTicksPerBar = 1920;

} // namespace

// NTSC-U/C: 0x003d6f10, PAL: 0x0040ee00
Message *SeekerMsg::New() {
    return new SeekerMsg;
}

// NTSC-U/C: 0x003dcce0, PAL: 0x00415118
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *SeekerMsg::Clone() {
    return new SeekerMsg(*this);
}

// NTSC-U/C: 0x003dcd50, PAL: 0x00415188
int SeekerMsg::Type() {
    return g_nSeekerMsgType;
}

// NTSC-U/C: 0x003dcd60, PAL: 0x00415198
const char *SeekerMsg::Name() {
    return "SeekerMsg";
}

// NTSC-U/C: 0x003d81f0, PAL: 0x00410568
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
