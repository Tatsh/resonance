#include "game/multiscreenanim.h"

#include <algorithm>
#include <cmath>

#include "game/player.h"
#include "rnd/mat.h"
#include "script/scripthost.h"

namespace {

constexpr float kStepPeriod = 480.0f;

// mPhase before SetFrame() applies the first step.
constexpr int kNoPhase = -1000;

// Room SetFrame() needs for one leader per even step.
constexpr int kMaxLeaders = 4;

constexpr int kStepsPerCycle = 8;

// A single leader appears in these two steps of the cycle.
constexpr int kSingleLeaderFirstStep = 0;
constexpr int kSingleLeaderSecondStep = 2;

// A player within this many points of the highest score is a leader.
constexpr int kLeaderMargin = 4;

// Script template the constructor runs, with the level.
constexpr int kLevelScriptTemplate = 1019;
constexpr int kInitialLevel = 1;

} // namespace

MultiScreenAnim::MultiScreenAnim(const std::vector<TnlArena::ScreenMesh> *pScreens,
                                 const std::vector<TnlArena::PlayerMaterial *> *pPlayerMaterials)
    : mScreens(pScreens), mPlayerMaterials(pPlayerMaterials), mPeriod(kStepPeriod),
      mPhase(kNoPhase) {
    mLeaderMats.reserve(kMaxLeaders);
    CallScriptTemplate(kLevelScriptTemplate, kInitialLevel);
}

MultiScreenAnim::~MultiScreenAnim() {
}

void MultiScreenAnim::SetFrame(float flFrame) {
    if (mLeaderMats.size() == 0) {
        return;
    }

    const int nPhase = static_cast<int>(floorf(flFrame / mPeriod));
    if (nPhase == mPhase) {
        return;
    }
    mPhase = nPhase;

    const int nStep = nPhase % kStepsPerCycle;
    Rnd::Mat *pMat = nullptr;
    if (mLeaderMats.size() == 1 &&
        (nStep == kSingleLeaderFirstStep || nStep == kSingleLeaderSecondStep)) {
        pMat = mLeaderMats[0];
    } else if ((nStep & 1) == 0 && static_cast<unsigned>(nStep / 2) < mLeaderMats.size()) {
        pMat = mLeaderMats[nStep / 2];
    }

    for (std::vector<TnlArena::ScreenMesh>::const_iterator it = mScreens->begin();
         it != mScreens->end();
         ++it) {
        it->SetMaterial(pMat != nullptr ? pMat : it->mMat);
    }
}

void MultiScreenAnim::UpdateLeaders() {
    mLeaderMats.erase(mLeaderMats.begin(), mLeaderMats.end());

    int nHighScore = 0;
    for (std::vector<TnlArena::PlayerMaterial *>::const_iterator it = mPlayerMaterials->begin();
         it != mPlayerMaterials->end();
         ++it) {
        nHighScore = std::max(nHighScore, (*it)->mPlayer->GetScore());
    }
    for (std::vector<TnlArena::PlayerMaterial *>::const_iterator it = mPlayerMaterials->begin();
         it != mPlayerMaterials->end();
         ++it) {
        if (nHighScore - (*it)->mPlayer->GetScore() < kLeaderMargin) {
            mLeaderMats.push_back((*it)->mMat);
        }
    }
}
