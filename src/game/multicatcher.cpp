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

// NTSC-U/C: 0x001b1a60, PAL: 0x001b7820
MultiCatcher::MultiCatcher(PhraseMgr *pPhraseMgr,
                           Quantizer *pQuantizer,
                           const TrackData *pTrackData,
                           Sch::TickClock *pClock,
                           Sch::Tick tick)
    : Catcher(pPhraseMgr, pQuantizer, pTrackData, pClock, kMultiCatcherFlag, tick) {
}

// NTSC-U/C: 0x001b0d98, PAL: 0x001b6b48
MultiCatcher::~MultiCatcher() {
}

// NTSC-U/C: 0x001ad8e8, PAL: 0x001b3650
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
                               mTrackData->GetCatchPoints(nBar),
                               nAutoCatch ^ 1);
    Send(&captured);

    SectionCapturedMsg section(nBar, nEnd, mTrack, mPlayer, nAutoCatch);
    Send(&section);
    mPhraseMgr->SetPhraseByte(nBar, static_cast<char>(nPoints));

    if (nPowerbar != kNoPowerbar && nAutoCatch == 0) {
        CaughtPowerbarMsg msg;
        msg.mKind = static_cast<HudItemKind>(nPowerbar);
        msg.mPlayer = mPlayer;
        Send(&msg);
        mPlayer->Handle(&msg);
    }
}

// NTSC-U/C: 0x001b0e00, PAL: 0x001b6bb0
void MultiCatcher::ReportCaughtPowerbar(int) {
}
