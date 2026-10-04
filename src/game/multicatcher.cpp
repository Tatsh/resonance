#include "game/multicatcher.h"

#include "game/player.h"
#include "game/trackdata.h"
#include "msg/caughtpowerbarmsg.h"
#include "msg/phrasecapturedmsg.h"
#include "msg/sectioncapturedmsg.h"

namespace {

// The value MultiCatcher supplies for Catcher's int parameter.
constexpr int kMultiCatcherFlag = 1;

// The multiplier an automatic catch scores with.
constexpr int kAutoCatchMultiplier = 1;

// PhraseMgr::GetPowerbar() reports this for a bar without a power bar.
constexpr int kNoPowerbar = -1;

} // namespace

MultiCatcher::MultiCatcher(PhraseMgr *pPhraseMgr,
                           Quantizer *pQuantizer,
                           const TrackData *pTrackData,
                           Sch::TickClock *pClock,
                           Sch::Tick tick)
    : Catcher(pPhraseMgr, pQuantizer, pTrackData, pClock, kMultiCatcherFlag, tick) {
}

MultiCatcher::~MultiCatcher() {
}

void MultiCatcher::CapturePhrase(int nBar, int nRun, int nAutoCatch) {
    const int nMultiplier = nAutoCatch != 0 ? kAutoCatchMultiplier : mPlayer->GetMultiplier(nBar);
    const int nRunBars = nRun - 1;
    const int nPowerbar = mPhraseMgr->GetPowerbar(nBar);
    const int nEnd = nBar + 1;
    const int nPoints = mTrackData->GetPoints(nBar) * nMultiplier;
    SetPhraseOwners(nBar, nEnd, mPlayer);

    PhraseCapturedMsg captured(nBar,
                               nEnd,
                               nBar - nRunBars,
                               nEnd,
                               mTrack,
                               mPlayer,
                               nPoints,
                               mTrackData->GetJuice(nBar),
                               nAutoCatch ^ 1);
    Send(&captured);

    SectionCapturedMsg section(nBar, nEnd, mTrack, mPlayer, nAutoCatch);
    Send(&section);
    mPhraseMgr->SetPhraseByte(nBar, static_cast<char>(nPoints));

    if (nPowerbar != kNoPowerbar && nAutoCatch == 0) {
        CaughtPowerbarMsg msg;
        msg.mKind = static_cast<PowerupType>(nPowerbar);
        msg.mPlayer = mPlayer;
        Send(&msg);
        mPlayer->Dispatch(&msg);
    }
}

void MultiCatcher::ReportCaughtPowerbar(int) {
}
