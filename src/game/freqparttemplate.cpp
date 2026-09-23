#include "game/freqparttemplate.h"

namespace {

// The identifier a template has until MetFreqMakerAssetManager::PollLoad() numbers it.
constexpr int kUnnumbered = -1;

} // namespace

// 0x002576c0
FreqPartTemplate::FreqPartTemplate(const HxStr &name, Rnd::Mat *pMaterial, int nScaleX, int nScaleZ)
    : mId(kUnnumbered), mName(name), mMaterial(pMaterial), mColorable(0), mRandomizable(0),
      mCategory(0), mScaleX(nScaleX), mScaleZ(nScaleZ) {
}

// 0x00257730
FreqPartTemplate::~FreqPartTemplate() {
}
