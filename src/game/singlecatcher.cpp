#include "game/singlecatcher.h"

#include "app/application.h"
#include "game/player.h"
#include "game/playmap.h"
#include "game/trackdata.h"
#include "msg/caughtpowerbarmsg.h"
#include "msg/phrasecapturedmsg.h"
#include "msg/sectioncapturedmsg.h"

namespace {

// The bars each seeker range spans, which SingleCatcher supplies for Catcher's int parameter.
constexpr int kSeekerBarCount = 2;

// PhraseMgr::GetPowerbar() reports this for a bar without a power bar.
constexpr int kNoPowerbar = -1;

} // namespace

// 0x001b19b0
SingleCatcher::SingleCatcher(PhraseMgr *pPhraseMgr,
                             Quantizer *pQuantizer,
                             const TrackData *pTrackData,
                             Sch::TickClock *pClock,
                             Sch::Tick tick)
    : Catcher(pPhraseMgr, pQuantizer, pTrackData, pClock, kSeekerBarCount, tick),
      mLastStep(Application::shared()->GetPlayMap()->mSteps.back()) {
}

// 0x001b0d30
SingleCatcher::~SingleCatcher() {
}

// 0x001ad630
void SingleCatcher::Slot9(int nBar, int nRun, int nAutoCatch) {
    const int nRunEnd = nBar + 1;
    const int nStepStart = mTrackData->StepStartBar(nBar);
    const int nStepEnd = mTrackData->FollowingStepBar(nBar);
    const int nRunStart = nBar - (nRun - 1);
    SetPhraseOwners(nStepStart, nStepEnd, mPlayer);

    const int nMultiplier = mPlayer->Slot16(nRunStart);
    int nPoints = 0;
    for (int nRunBar = nRunStart; nRunBar < nRunEnd; ++nRunBar) {
        nPoints += mTrackData->GetPoints(nRunBar);
    }
    PhraseCapturedMsg captured(nStepStart,
                               nStepEnd,
                               nRunStart,
                               nRunEnd,
                               mTrack,
                               mPlayer,
                               nPoints * nMultiplier,
                               mTrackData->GetUnknown08(nStepStart),
                               nAutoCatch ^ 1);
    Send(&captured);

    SectionCapturedMsg section(nStepStart, nStepEnd, mTrack, mPlayer, nAutoCatch);
    Send(&section);
}

// 0x001ad840
void SingleCatcher::Slot10(int nBar) {
    const int nPowerbar = mPhraseMgr->GetPowerbar(nBar);
    if (nPowerbar == kNoPowerbar) {
        return;
    }

    CaughtPowerbarMsg msg;
    msg.mKind = static_cast<HudItemKind>(nPowerbar);
    msg.mPlayer = mPlayer;
    Send(&msg);
    mPlayer->Handle(&msg);
}

// 0x001b1a40
void SingleCatcher::ResetOwners(int, Player *pPlayer) {
    mPhraseMgr->ResetOwners(pPlayer);
}
