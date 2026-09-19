#include "rnd/pscam.h"

#include "rnd/cam.h"
#include "rnd/tex.h"

namespace Rnd {

namespace {

// Vertical ratio of a four by three display, which is what a camera with no render target
// projects into.
constexpr float kDisplayYRatio = 0.75f;

} // namespace

void PsCam::UpdateTargetAspect() {
    if (mpTargetTex == nullptr) {
        mYRatio = kDisplayYRatio;
    }
    Cam::UpdateTargetAspect();
}

void PsCam::SetTargetTex(Tex *pTex) {
    Cam::SetTargetTex(pTex);
    if (pTex == nullptr) {
        mYRatio = kDisplayYRatio;
    }
}

} // namespace Rnd
