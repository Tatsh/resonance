#include "game/pitchingstg.h"

// 0x001c4c68
PitchingSTG::~PitchingSTG() {
    PitchingSTG::Slot3(); // The binary calls this class's own body rather than dispatching.
    delete mPitcher;
    delete mJamEffects;
}

// 0x001c4798
void PitchingSTG::Slot4(MsgSource *pPrimary, MsgSource *pOptional, MsgSource *pSecondary) {
    pPrimary->AddSink(mMixer);
    pPrimary->AddSink(mPitcher);
    pPrimary->AddSink(mPhraseMgr);
    pPrimary->AddSink(mPhrasePlayer);
    if (mJamEffects != nullptr) {
        pPrimary->AddSink(mJamEffects);
    }

    mPitcher->AddSink(mPhrasePlayer);
    mPitcher->AddSink(mMuseSynth);

    if (pOptional != nullptr) {
        pOptional->AddSink(mPhraseMgr);
    }

    pSecondary->AddSink(mPhraseMgr);
    pSecondary->AddSink(mPitcher);

    mPhrasePlayer->AddSink(mMuseSynth);
}

// 0x001c4d60
void PitchingSTG::Slot6(int nValue) {
    mMuseSynth->AddMuseSink(mMixer);
    mMixer->mUnknown04 = nValue;
}

// 0x001c4da0
void PitchingSTG::Slot7(MsgSink *pSink) {
    mPhraseMgr->AddSink(pSink);
    mPitcher->AddSink(pSink);
    if (mJamEffects != nullptr) {
        mJamEffects->AddSink(pSink);
    }
}

// 0x001c4e30
void PitchingSTG::Slot8(MsgSink *pSink) {
    if (pSink != nullptr) {
        mPhraseMgr->mNetSink = pSink;
    }
}
