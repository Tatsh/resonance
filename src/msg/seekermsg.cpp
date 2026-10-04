#include "msg/seekermsg.h"

#include <iostream>

#include "game/player.h"
#include "os/hxstr.h"

namespace {

constexpr int kTicksPerBar = 1920;

} // namespace

Message *SeekerMsg::New() {
    return new SeekerMsg;
}

Message *SeekerMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new SeekerMsg(*this);
}

int SeekerMsg::Type() {
    return g_nSeekerMsgType;
}

const char *SeekerMsg::GetName() const {
    return "SeekerMsg";
}

void SeekerMsg::PrintExtra(std::ostream &stream) const {
    // The colour name is copied into a temporary before it is written.
    stream << HxStr(mPlayer->mColorName);
    if (mEnabled != 0) {
        std::ostream &rest = stream << " bars[" << mFirstBar << " - " << mFirstBar + mBarCount
                                    << "]" << " tr#" << mTrack << " when: ";
        // The discarded call is the shape of an assertion compiled without its report.
        Sch::Tick::IsInRange(kTicksPerBar);
        rest << mWhen.mTick / kTicksPerBar;
    } else {
        stream << " (off)";
    }
}
