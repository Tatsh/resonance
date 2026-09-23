#include "game/voxingstg.h"

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
