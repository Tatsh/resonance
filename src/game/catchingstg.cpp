#include "game/catchingstg.h"

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/multicatcher.h"
#include "game/singlecatcher.h"
#include "mid/tick.h"
#include "sch/tempomap.h"
#include "script/configquery.h"

namespace {

// The configuration code the catch window comes from, in milliseconds.
constexpr int kCatchWindowConfigCode = 924;

constexpr long long kNanosecondsPerMillisecond = 1000000;

// The phrase manager's export lead grows by this many ticks for each track index.
constexpr int kExportLeadStep = 120;

} // namespace

CatchingSTG::CatchingSTG(TrackData *pTrackData)
    : ScoreTrackGraph(pTrackData), mNeutralizer(nullptr), mCatcher(nullptr) {
    mNeutralizer = new PhraseNeutralizer(mTrackData, mPhraseMgr);

    const long long nWindowNs =
        QueryConfigValue(kCatchWindowConfigCode) * kNanosecondsPerMillisecond;
    const Sch::TempoMap *pTempo = mApplication->GetSongClock()->mTempoMap;
    const Sch::Tick window(
        static_cast<int>((nWindowNs + pTempo->mCeilingBias) / pTempo->mNanosecondsPerTick));

    if (mApplication->GetGameMode() == kGameModeSolo) {
        mCatcher = new SingleCatcher(
            mPhraseMgr, mQuantizer, mTrackData, mApplication->GetSongClock(), window);
    } else {
        mCatcher = new MultiCatcher(
            mPhraseMgr, mQuantizer, mTrackData, mApplication->GetSongClock(), window);
    }

    mPhraseMgr->mExportLead = Sch::Tick((mTrack * kExportLeadStep) + kExportLeadStep);
}

CatchingSTG::~CatchingSTG() {
    CatchingSTG::Stop(); // The binary calls this class's body rather than dispatching.
    delete mCatcher;
    delete mNeutralizer;
}

void CatchingSTG::Start() {
    ScoreTrackGraph::Start();
    mCatcher->Start();
}

void CatchingSTG::Stop() {
    mCatcher->Stop();
    ScoreTrackGraph::Stop();
}

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

void CatchingSTG::ConnectGamer(MsgSource *pSource) {
    pSource->AddSink(mMixer);
}

void CatchingSTG::SetMixerOutput(MsgSink *pOutput) {
    mMuseSynth->AddSink(mMixer);
    mMixer->mOutput = pOutput;
}

void CatchingSTG::ConnectToTunnel(MsgSink *pSink) {
    mPhraseMgr->AddSink(pSink);
    mCatcher->AddSink(pSink);
    mNeutralizer->AddSink(pSink);
}

void CatchingSTG::SetNetSink(MsgSink *pSink) {
    if (pSink != nullptr) {
        mPhraseMgr->mNetSink = pSink;
    }
}

int CatchingSTG::HasNothingPending() {
    return mCatcher->IsPhraseRunEmpty();
}

void CatchingSTG::GivePhrases(int nTick, Player *pPlayer) {
    // Yes, the binary discards this call's result. It is what remains of a compiled-away assertion.
    mApplication->GetGameMode();

    // Unguarded on purpose: a MultiCatcher yields a null receiver here.
    dynamic_cast<SingleCatcher *>(mCatcher)->ResetOwners(nTick, pPlayer);
}

int CatchingSTG::CanGivePhrases() {
    return 1;
}

void CatchingSTG::CreatePowerbarMgr() {
    mPhraseMgr->CreatePowerbarMgr();
}
