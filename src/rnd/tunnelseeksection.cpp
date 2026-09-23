#include "rnd/tunnelseeksection.h"

namespace Rnd {

// 0x0046ed38
TunnelSeekSection::TunnelSeekSection() : mRing(-1), mSlice(kTunnelSeekNoSlice) {
}

// 0x004780d0
void TunnelSeekSection::Set(int nSlice, int nRing, int bEndCap, int bStartCap) {
    mSlice = nSlice;
    mDirty = 1;
    mRing = nRing;
    mEndCap = bEndCap;
    mStartCap = bStartCap;
}

// 0x004780f0
void TunnelSeekSection::Invalidate() {
    mSlice = kTunnelSeekNoSlice;
    mDirty = 1;
}

} // namespace Rnd
