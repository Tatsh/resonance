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

// 0x001b1a60
MultiCatcher::MultiCatcher(PhraseMgr *pPhraseMgr,
                           Quantizer *pQuantizer,
                           const TrackData *pTrackData,
                           Sch::TickClock *pClock,
                           Sch::Tick tick)
    : Catcher(pPhraseMgr, pQuantizer, pTrackData, pClock, kMultiCatcherFlag, tick) {
}

// 0x001b0d98
MultiCatcher::~MultiCatcher() {
}

// 0x001ad8e8
void MultiCatcher::Slot9(int nFirst, int nSecond, int nThird) {
    const int nMultiplier = nThird != 0 ? kAutoCatchMultiplier : mPlayer->Slot16(nFirst);
    const int nRunBars = nSecond - 1;
    const int nPowerbar = mPhraseMgr->GetPowerbar(nFirst);
    const int nEnd = nFirst + 1;
    const int nPoints = mTrackData->GetPoints(nFirst) * nMultiplier;
    SetPhraseOwners(nFirst, nEnd, mPlayer);

    PhraseCapturedMsg captured(nFirst,
                               nEnd,
                               nFirst - nRunBars,
                               nEnd,
                               mTrack,
                               mPlayer,
                               nPoints,
                               mTrackData->GetUnknown08(nFirst),
                               nThird ^ 1);
    Send(&captured);

    SectionCapturedMsg section(nFirst, nEnd, mTrack, mPlayer, nThird);
    Send(&section);
    mPhraseMgr->SetPhraseByte(nFirst, static_cast<char>(nPoints));

    if (nPowerbar != kNoPowerbar && nThird == 0) {
        CaughtPowerbarMsg msg;
        msg.mKind = static_cast<HudItemKind>(nPowerbar);
        msg.mPlayer = mPlayer;
        Send(&msg);
        mPlayer->Handle(&msg);
    }
}

// 0x001b0e00
void MultiCatcher::Slot10(int) {
}
