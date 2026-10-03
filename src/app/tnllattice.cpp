#include "app/tnllattice.h"

#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/matanim.h"

namespace {

// Start frame the constructor writes, far beyond a song's length.
constexpr float kNoFrame = 1e9f;

// Longest time since the start the animation is driven to.
constexpr float kMaxAnimFrame = 8160.0f;

} // namespace

// NTSC-U/C: 0x0043c598, PAL: 0x00478bb8
TnlLattice::TnlLattice() : mStartFrame(kNoFrame) {
    mMatAnim = dynamic_cast<Rnd::MatAnim *>(Rnd::TheManager.Find(HxStr("lattice.mnm")));
    mMatAnim->SetFrame(0.0f);
}

// NTSC-U/C: 0x00456368, PAL: 0x00493898
TnlLattice::~TnlLattice() {
    mMatAnim->SetFrame(0.0f);
}

// NTSC-U/C: 0x004563c0, PAL: 0x004938f0
void TnlLattice::SetFrame(float flFrame) {
    if (flFrame < mStartFrame) {
        return;
    }
    float flAnimFrame = flFrame - mStartFrame;
    if (kMaxAnimFrame < flAnimFrame) {
        flAnimFrame = kMaxAnimFrame;
    }
    mMatAnim->SetFrame(flAnimFrame);
}
