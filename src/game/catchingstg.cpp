#include "game/catchingstg.h"

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/multicatcher.h"
#include "game/singlecatcher.h"
#include "mid/mbt.h"
#include "sch/tempomap.h"
#include "script/configquery.h"

namespace {

// The configuration code the catch window comes from, in milliseconds.
constexpr int kCatchWindowConfigCode = 0x39c;

constexpr long long kNanosecondsPerMillisecond = 1000000;

// The phrase manager's export lead grows by this many ticks for each track index.
constexpr int kExportLeadStep = 120;

} // namespace

// 0x0019fb80
CatchingSTG::CatchingSTG(TrackData *pTrackData)
    : ScoreTrackGraph(pTrackData), mNeutralizer(nullptr), mCatcher(nullptr) {
    mNeutralizer = new PhraseNeutralizer(mTrackData, mPhraseMgr);

    const long long nWindowNs =
        QueryConfigValue(kCatchWindowConfigCode) * kNanosecondsPerMillisecond;
    const Sch::TempoMap *pTempo = mApplication->GetSongClock()->mTempoMap;
    const Mid::MBT window(
        static_cast<int>((nWindowNs + pTempo->mCeilingBias) / pTempo->mNanosecondsPerTick));

    // The window is in MIDI ticks, and the binary passes it sign-extended as the Sch::Tick count.
    if (mApplication->GetGameMode() == kGameModeSolo) {
        mCatcher = new SingleCatcher(mPhraseMgr,
                                     mQuantizer,
                                     mTrackData,
                                     mApplication->GetSongClock(),
                                     Sch::Tick{window.mTick});
    } else {
        mCatcher = new MultiCatcher(mPhraseMgr,
                                    mQuantizer,
                                    mTrackData,
                                    mApplication->GetSongClock(),
                                    Sch::Tick{window.mTick});
    }

    mPhraseMgr->mExportLead = Mid::MBT((mUnknown00 * kExportLeadStep) + kExportLeadStep);
}

// 0x001a0450
CatchingSTG::~CatchingSTG() {
    CatchingSTG::Slot3(); // The binary calls this class's own body rather than dispatching.
    delete mCatcher;
    delete mNeutralizer;
}

// 0x001a04d8
void CatchingSTG::Slot2() {
    ScoreTrackGraph::Slot2();
    mCatcher->Slot4();
}

// 0x001a0518
void CatchingSTG::Slot3() {
    mCatcher->Slot5();
    ScoreTrackGraph::Slot3();
}

// 0x0019fd88
void CatchingSTG::Slot4(MsgSource *pPrimary, MsgSource *pOptional, MsgSource *pSecondary) {
    pPrimary->AddSink(mMixer);
    pPrimary->AddSink(mCatcher);
    pPrimary->AddSink(mNeutralizer);
    pPrimary->AddSink(mPhrasePlayer);
    pPrimary->AddSink(mPhraseMgr);

    mCatcher->AddSink(mPhrasePlayer);
    mCatcher->AddSink(mMuseSynth);

    pSecondary->AddSink(mPhraseMgr);
    pSecondary->AddSink(mCatcher);

    if (pOptional != nullptr) {
        pOptional->AddSink(mPhraseMgr);
        pOptional->AddSink(mCatcher);
    }

    mPhrasePlayer->AddSink(mMuseSynth);
}

// 0x001a0558
void CatchingSTG::Slot5(MsgSource *pSource) {
    pSource->AddSink(mMixer);
}

// 0x001a0588
void CatchingSTG::Slot6(MsgSink *pOutput) {
    mMuseSynth->AddSink(mMixer);
    mMixer->mOutput = pOutput;
}

// 0x001a05c8
void CatchingSTG::Slot7(MsgSink *pSink) {
    mPhraseMgr->AddSink(pSink);
    mCatcher->AddSink(pSink);
    mNeutralizer->AddSink(pSink);
}

// 0x001a0650
void CatchingSTG::Slot8(MsgSink *pSink) {
    if (pSink != nullptr) {
        mPhraseMgr->mNetSink = pSink;
    }
}

// 0x001a06f0
int CatchingSTG::Slot9() {
    return mCatcher->Slot6();
}

// 0x001a0668
void CatchingSTG::Slot10(int nTick, Player *pPlayer) {
    // Yes, the binary discards this call's result. It is what remains of a compiled-away assertion.
    mApplication->GetGameMode();

    // Unguarded on purpose: a MultiCatcher yields a null receiver here.
    dynamic_cast<SingleCatcher *>(mCatcher)->ResetOwners(nTick, pPlayer);
}

// 0x001a0428
int CatchingSTG::Slot11() {
    return 1;
}

// 0x001a0720
void CatchingSTG::Slot12() {
    mPhraseMgr->CreatePowerbarMgr();
}
