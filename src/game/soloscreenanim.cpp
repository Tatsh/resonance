#include "game/soloscreenanim.h"

#include <cmath>

#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "script/scripthost.h"

namespace {

constexpr float kBlinkPeriod = 960.0f;

// mPhase before SetFrame() applies the first period.
constexpr int kNoPhase = -10000;

enum Level {
    kLevelNoise = 0,
    kLevelOwn = 1,
    kLevelBlink = 2,
};

// Script template SetLevel() runs with the new level.
constexpr int kLevelScriptTemplate = 1019;

} // namespace

// NTSC-U/C: 0x004066d0, PAL: 0x0043fff0
SoloScreenAnim::SoloScreenAnim(const std::vector<TnlArena::ScreenMesh> *pScreens,
                               Rnd::Mat *pPlayerMat)
    : mPeriod(kBlinkPeriod), mPhase(kNoPhase), mLevel(kLevelOwn), mPlayerMat(pPlayerMat),
      mNoiseMat(dynamic_cast<Rnd::Mat *>(Rnd::TheManager.Find(HxStr("noise.mat")))),
      mScreens(pScreens) {
}

// NTSC-U/C: 0x0040c690, PAL: 0x004460b8
SoloScreenAnim::~SoloScreenAnim() {
}

// NTSC-U/C: 0x0040c7f8, PAL: 0x00446220
void SoloScreenAnim::SetFrame(float flFrame) {
    if (mLevel != kLevelBlink) {
        return;
    }

    const int nPhase = static_cast<int>(floorf(flFrame / mPeriod));
    if (nPhase == mPhase) {
        return;
    }
    mPhase = nPhase;
    for (std::vector<TnlArena::ScreenMesh>::const_iterator it = mScreens->begin();
         it != mScreens->end();
         ++it) {
        it->SetMaterial((nPhase & 1) == 0 ? mPlayerMat : it->mMat);
    }
}

// NTSC-U/C: 0x0040c738, PAL: 0x00446160
void SoloScreenAnim::SetLevel(int nLevel) {
    if (nLevel > kLevelBlink) {
        nLevel = kLevelBlink;
    }
    if (nLevel < kLevelNoise) {
        nLevel = kLevelNoise;
    }
    if (nLevel == mLevel) {
        return;
    }

    mLevel = nLevel;
    for (std::vector<TnlArena::ScreenMesh>::const_iterator it = mScreens->begin();
         it != mScreens->end();
         ++it) {
        if (mLevel == kLevelNoise) {
            it->SetMaterial(mNoiseMat);
        } else if (mLevel == kLevelOwn) {
            it->SetMaterial(it->mMat);
        } else {
            it->SetMaterial(mPlayerMat);
        }
    }
    CallScriptTemplate(kLevelScriptTemplate, mLevel);
}
