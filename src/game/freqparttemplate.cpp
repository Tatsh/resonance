#include "game/freqparttemplate.h"

namespace {

// The identifier a template has until MetFreqMakerAssetManager::PollLoad() numbers it.
constexpr int kUnnumbered = -1;

} // namespace

FreqPartTemplate::FreqPartTemplate(const HxStr &name, Rnd::Mat *pMaterial, int nScaleX, int nScaleZ)
    : mId(kUnnumbered), mName(name), mMaterial(pMaterial), mColorable(0), mRandomizable(0),
      mCategory(0), mScaleX(nScaleX), mScaleZ(nScaleZ) {
}

FreqPartTemplate::~FreqPartTemplate() {
}
