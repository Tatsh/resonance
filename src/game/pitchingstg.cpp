#include "game/pitchingstg.h"

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "mid/mbt.h"

// 0x001c45b0
PitchingSTG::PitchingSTG(TrackData *pTrackData)
    : ScoreTrackGraph(pTrackData), mPitcher(nullptr), mJamEffects(nullptr) {
    if (mTrackData->mKind == kTrackModeRiff) {
        mPitcher = new NotePitcher(mPhraseMgr,
                                   mQuantizer,
                                   mApplication->GetSongClock(),
                                   mTrackData,
                                   mApplication->GetPlayMode() == kPlayModeGame,
                                   1,
                                   0);
    } else if (mTrackData->mKind == kTrackModeScratch) {
        mPitcher = new Scratcher(mPhraseMgr, mQuantizer, mApplication->GetSongClock(), mTrackData);
    }

    if (mApplication->GetPlayMode() == kPlayModeJam) {
        mJamEffects = new JamEffectsMgr(
            mUnknown00, mTrackData->mChannel, mApplication->GetPlayMap(), mPhraseMgr, mMuseSynth);
        mPhrasePlayer->SetJamEffectsMgr(mJamEffects);
    }
}

// 0x001c4cf0
void PitchingSTG::Slot2() {
    ScoreTrackGraph::Slot2();
    mPitcher->Start(kMBTInfinity); // Unguarded, as in the binary, for a track of any other kind.
}

// 0x001c4d28
void PitchingSTG::Slot3() {
    mPitcher->Stop();
    ScoreTrackGraph::Slot3();
}

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
void PitchingSTG::Slot6(MsgSink *pOutput) {
    mMuseSynth->AddSink(mMixer);
    mMixer->mOutput = pOutput;
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
