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
constexpr int kCatchWindowConfigCode = 924;

constexpr long long kNanosecondsPerMillisecond = 1000000;

// The phrase manager's export lead grows by this many ticks for each track index.
constexpr int kExportLeadStep = 120;

} // namespace

// NTSC-U/C: 0x0019fb80, PAL: 0x001a58e8
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

    mPhraseMgr->mExportLead = Mid::MBT((mTrack * kExportLeadStep) + kExportLeadStep);
}

// NTSC-U/C: 0x001a0450, PAL: 0x001a61b8
CatchingSTG::~CatchingSTG() {
    CatchingSTG::Stop(); // The binary calls this class's body rather than dispatching.
    delete mCatcher;
    delete mNeutralizer;
}

// NTSC-U/C: 0x001a04d8, PAL: 0x001a6240
void CatchingSTG::Start() {
    ScoreTrackGraph::Start();
    mCatcher->Start();
}

// NTSC-U/C: 0x001a0518, PAL: 0x001a6280
void CatchingSTG::Stop() {
    mCatcher->Stop();
    ScoreTrackGraph::Stop();
}

// NTSC-U/C: 0x0019fd88, PAL: 0x001a5af0
void CatchingSTG::ConnectInputs(MsgSource *pPrimary, MsgSource *pOptional, MsgSource *pSecondary) {
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

// NTSC-U/C: 0x001a0558, PAL: 0x001a62c0
void CatchingSTG::ConnectGamer(MsgSource *pSource) {
    pSource->AddSink(mMixer);
}

// NTSC-U/C: 0x001a0588, PAL: 0x001a62f0
void CatchingSTG::SetMixerOutput(MsgSink *pOutput) {
    mMuseSynth->AddSink(mMixer);
    mMixer->mOutput = pOutput;
}

// NTSC-U/C: 0x001a05c8, PAL: 0x001a6330
void CatchingSTG::ConnectToTunnel(MsgSink *pSink) {
    mPhraseMgr->AddSink(pSink);
    mCatcher->AddSink(pSink);
    mNeutralizer->AddSink(pSink);
}

// NTSC-U/C: 0x001a0650, PAL: 0x001a63b8
void CatchingSTG::SetNetSink(MsgSink *pSink) {
    if (pSink != nullptr) {
        mPhraseMgr->mNetSink = pSink;
    }
}

// NTSC-U/C: 0x001a06f0, PAL: 0x001a6458
int CatchingSTG::HasNothingPending() {
    return mCatcher->IsPhraseRunEmpty();
}

// NTSC-U/C: 0x001a0668, PAL: 0x001a63d0
void CatchingSTG::GivePhrases(int nTick, Player *pPlayer) {
    // Yes, the binary discards this call's result. It is what remains of a compiled-away assertion.
    mApplication->GetGameMode();

    // Unguarded on purpose: a MultiCatcher yields a null receiver here.
    dynamic_cast<SingleCatcher *>(mCatcher)->ResetOwners(nTick, pPlayer);
}

// NTSC-U/C: 0x001a0428, PAL: 0x001a6190
int CatchingSTG::CanGivePhrases() {
    return 1;
}

// NTSC-U/C: 0x001a0720, PAL: 0x001a6488
void CatchingSTG::CreatePowerbarMgr() {
    mPhraseMgr->CreatePowerbarMgr();
}
