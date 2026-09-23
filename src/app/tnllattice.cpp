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

TnlLattice::TnlLattice() : mStartFrame(kNoFrame) {
    mMatAnim = dynamic_cast<Rnd::MatAnim *>(Rnd::g_manager.Find(HxStr("lattice.mnm")));
    mMatAnim->SetFrame(0.0f);
}

TnlLattice::~TnlLattice() {
    mMatAnim->SetFrame(0.0f);
}

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
