#include "game/voxingstg.h"

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "mid/tick.h"

VoxingSTG::VoxingSTG(TrackData *pTrackData)
    : ScoreTrackGraph(pTrackData), mVoxer(nullptr), mJamEffects(nullptr) {
    mVoxer = new Voxer(mPhraseMgr, mQuantizer, mApplication->GetSongClock(), mTrackData);
    mOldGemMaker = new AxeOldGemMaker(mTrackData);
    mNewGemMaker = new AxeNewGemMaker(mTrackData);

    if (mApplication->GetPlayMode() == kPlayModeJam) {
        mJamEffects = new JamEffectsMgr(
            mTrack, mTrackData->mChannel, mApplication->GetPlayMap(), mPhraseMgr, mMuseSynth);
        mPhrasePlayer->SetJamEffectsMgr(mJamEffects);
    }
}

void VoxingSTG::Start() {
    ScoreTrackGraph::Start();
    mVoxer->Start(kTickInfinity);
}

void VoxingSTG::Stop() {
    mVoxer->Stop();
    ScoreTrackGraph::Stop();
}

VoxingSTG::~VoxingSTG() {
    VoxingSTG::Stop(); // The binary calls this class's body rather than dispatching.
    delete mNewGemMaker;
    delete mOldGemMaker;
    delete mVoxer;
    delete mJamEffects;
}

void VoxingSTG::ConnectInputs(MsgSource *pPrimary, MsgSource *pOptional, MsgSource *pSecondary) {
    pPrimary->AddSink(mNewGemMaker);
    pPrimary->AddSink(mMixer);
    pPrimary->AddSink(mVoxer);
    pPrimary->AddSink(mPhraseMgr);
    pPrimary->AddSink(mPhrasePlayer);
    if (mJamEffects != nullptr) {
        pPrimary->AddSink(mJamEffects);
    }

    mVoxer->AddSink(mMuseSynth);
    mVoxer->AddSink(mNewGemMaker);

    if (pOptional != nullptr) {
        pOptional->AddSink(mPhraseMgr);
    }

    pSecondary->AddSink(mPhraseMgr);
    pSecondary->AddSink(mVoxer);

    mPhraseMgr->AddSink(mOldGemMaker);

    mPhrasePlayer->AddSink(mMuseSynth);
}

void VoxingSTG::SetMixerOutput(MsgSink *pOutput) {
    mMuseSynth->AddSink(mMixer);
    mMixer->mOutput = pOutput;
}

void VoxingSTG::ConnectToTunnel(MsgSink *pSink) {
    mPhraseMgr->AddSink(pSink);
    mVoxer->AddSink(pSink);
    if (mJamEffects != nullptr) {
        mJamEffects->AddSink(pSink);
    }
    mOldGemMaker->AddSink(pSink);
    mNewGemMaker->AddSink(pSink);
}

void VoxingSTG::SetNetSink(MsgSink *pSink) {
    if (pSink != nullptr) {
        mPhraseMgr->mNetSink = pSink;
    }
}
