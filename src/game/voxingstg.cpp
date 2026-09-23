#include "game/voxingstg.h"

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "mid/mbt.h"

// 0x001da050
VoxingSTG::VoxingSTG(const TrackData *pTrackData)
    : ScoreTrackGraph(pTrackData), mVoxer(nullptr), mJamEffects(nullptr) {
    mVoxer = new Voxer(mPhraseMgr, mQuantizer, mApplication->GetSongClock(), mTrackData);
    mOldGemMaker = new AxeOldGemMaker(mTrackData);
    mNewGemMaker = new AxeNewGemMaker(mTrackData);

    if (mApplication->GetPlayMode() == kPlayModeJam) {
        mJamEffects = new JamEffectsMgr(
            mUnknown00, mTrackData->mChannel, mApplication->GetPlayMap(), mPhraseMgr, mMuseSynth);
        mPhrasePlayer->SetJamEffectsMgr(mJamEffects);
    }
}

// 0x001da7f8
void VoxingSTG::Slot2() {
    ScoreTrackGraph::Slot2();
    mVoxer->Start(kMBTInfinity);
}

// 0x001da830
void VoxingSTG::Slot3() {
    mVoxer->Stop();
    ScoreTrackGraph::Slot3();
}

// 0x001da730
VoxingSTG::~VoxingSTG() {
    VoxingSTG::Slot3(); // The binary calls this class's own body rather than dispatching.
    delete mNewGemMaker;
    delete mOldGemMaker;
    delete mVoxer;
    delete mJamEffects;
}

// 0x001da230
void VoxingSTG::Slot4(MsgSource *pPrimary, MsgSource *pOptional, MsgSource *pSecondary) {
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

// 0x001da868
void VoxingSTG::Slot6(MsgSink *pOutput) {
    mMuseSynth->AddSink(mMixer);
    mMixer->mOutput = pOutput;
}

// 0x001da8a8
void VoxingSTG::Slot7(MsgSink *pSink) {
    mPhraseMgr->AddSink(pSink);
    mVoxer->AddSink(pSink);
    if (mJamEffects != nullptr) {
        mJamEffects->AddSink(pSink);
    }
    mOldGemMaker->AddSink(pSink);
    mNewGemMaker->AddSink(pSink);
}

// 0x001da978
void VoxingSTG::Slot8(MsgSink *pSink) {
    if (pSink != nullptr) {
        mPhraseMgr->mNetSink = pSink;
    }
}
