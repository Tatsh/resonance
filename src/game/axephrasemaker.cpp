#include "game/axephrasemaker.h"

#include <algorithm>

#include "app/application.h"
#include "game/nullplayer.h"
#include "mid/mbt.h"
#include "msg/seekermsg.h"
#include "script/configquery.h"
#include "synth/ps2hardsynth.h"

namespace {

// The origin every guitar phrase maker reports, in MIDI ticks.
constexpr int kPeriodOrigin = 6;

// One bar at 480 ticks per quarter note.
constexpr int kBarTicks = 1920;

// The bar mPhraseBar holds before the first phrase.
constexpr int kNoBar = -1;

// The axis value the constructor assumes, the middle of the range.
constexpr float kAxisCenter = 0.5f;

// The two configuration codes that decide mSwitchBanks.
constexpr int kBankSwitchConfigCode = 0x3a4;
constexpr int kBankSwitchOverrideConfigCode = 0x3a1;

// A computed position, clamped to the finite range as the inline Mid::MBT arithmetic does.
inline Mid::MBT MakePosition(int nTick) {
    return Mid::MBT(std::min(std::max(nTick, kMBTMinimum), kMBTMaximum));
}

} // namespace

// 0x0019b5d0
AxePhraseMaker::AxePhraseMaker(PhraseMgr *pPhraseMgr,
                               Quantizer *pQuantizer,
                               const TrackData *pTrackData,
                               Sch::TickClock *)
    : mPhraseMgr(pPhraseMgr), mQuantizer(pQuantizer), mTrack(pTrackData->mUnknown04),
      mChannel(pTrackData->mChannel), mPhrase(nullptr), mPhraseBar(kNoBar), mPlayer(&g_nullPlayer),
      mBarTicks(kBarTicks), mTrackData(pTrackData), mValue(kAxisCenter) {
    mSwitchBanks = 0;
    if (QueryConfigFlag(kBankSwitchConfigCode) != 0) {
        mSwitchBanks = QueryConfigFlag(kBankSwitchOverrideConfigCode) == 0;
    }
}

// 0x0019bbd8
void AxePhraseMaker::RecordMuseMsg(MuseMsg *pMsg) {
    const Mid::MBT barStart = MakePosition(mBarTicks.mTick * mPhraseBar);
    const Mid::MBT offset = MakePosition(pMsg->mTick - barStart.mTick);
    (void)Mid::MBT(0); // Yes, the binary discards this position.
    mPhrase->AddMuseMsg(offset.mTick, pMsg);
    mPhrase->AddValue(offset.mTick, mValue);
}

// 0x0019c368
void AxePhraseMaker::PostSeekerMsg(int) {
    if (mPlayer->IsNull()) {
        return;
    }
    SeekerMsg msg(mPlayer);
    Send(&msg);
}

// 0x0019d438
int AxePhraseMaker::Slot5() {
    return Mid::MBT(kPeriodOrigin).mTick;
}

// 0x0019d860
void AxePhraseMaker::OnTrackSelect(TrackSelectMsg *pMsg) {
    if (pMsg->mUnknown04 != mTrack || pMsg->mUnknown08 != 0) {
        return;
    }
    mPlayer = pMsg->mUnknown10;
    if (mPlayer->IsNull()) {
        return;
    }
    PostSeekerMsg(pMsg->mPosition.mTick / mBarTicks.mTick);
}

// 0x0019d8f0
void AxePhraseMaker::OnInvalidateSeeker(InvalidateSeekerMsg *pMsg) {
    if (pMsg->mUnknown08 == mTrack) {
        PostSeekerMsg(pMsg->mUnknown04);
    }
}

// 0x0019d920
void AxePhraseMaker::OnSustainNote(SustainNoteMsg *pMsg) {
    StartPhrase(pMsg->mTick);
    RecordMuseMsg(pMsg);
}

// 0x0019d990
void AxePhraseMaker::Slot4(int nBar) {
    if ((nBar - 1) == mPhraseBar) {
        FinishPhrase();
    }
    PostSeekerMsg(nBar);
    if (mSwitchBanks == 0) {
        return;
    }
    if (mTrackData->IsStepStart(nBar) == 0) {
        return;
    }
    Application::shared()->GetSynth()->SelectBank(mChannel, mTrackData->FindStepIndex(nBar));
}

// 0x0019da58
int AxePhraseMaker::IsBarPlayable(int nBar) {
    int bPlayable = 0;
    if (mTrackData->QueryBar(nBar) != 0) {
        bPlayable = mPlayer->Slot9(nBar) != 0;
    }
    return bPlayable;
}
