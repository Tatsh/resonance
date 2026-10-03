#include "game/voxingstg.h"

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "mid/mbt.h"

// NTSC-U/C: 0x001da050, PAL: 0x001dffc0
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

// NTSC-U/C: 0x001da7f8, PAL: 0x001e0768
void VoxingSTG::Start() {
    ScoreTrackGraph::Start();
    mVoxer->Start(kMBTInfinity);
}

// NTSC-U/C: 0x001da830, PAL: 0x001e07a0
void VoxingSTG::Stop() {
    mVoxer->Stop();
    ScoreTrackGraph::Stop();
}

// NTSC-U/C: 0x001da730, PAL: 0x001e06a0
VoxingSTG::~VoxingSTG() {
    VoxingSTG::Stop(); // The binary calls this class's body rather than dispatching.
    delete mNewGemMaker;
    delete mOldGemMaker;
    delete mVoxer;
    delete mJamEffects;
}

// NTSC-U/C: 0x001da230, PAL: 0x001e01a0
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

// NTSC-U/C: 0x001da868, PAL: 0x001e07d8
void VoxingSTG::SetMixerOutput(MsgSink *pOutput) {
    mMuseSynth->AddSink(mMixer);
    mMixer->mOutput = pOutput;
}

// NTSC-U/C: 0x001da8a8, PAL: 0x001e0818
void VoxingSTG::ConnectToTunnel(MsgSink *pSink) {
    mPhraseMgr->AddSink(pSink);
    mVoxer->AddSink(pSink);
    if (mJamEffects != nullptr) {
        mJamEffects->AddSink(pSink);
    }
    mOldGemMaker->AddSink(pSink);
    mNewGemMaker->AddSink(pSink);
}

// NTSC-U/C: 0x001da978, PAL: 0x001e08e8
void VoxingSTG::SetNetSink(MsgSink *pSink) {
    if (pSink != nullptr) {
        mPhraseMgr->mNetSink = pSink;
    }
}
