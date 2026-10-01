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
            mTrack, mTrackData->mChannel, mApplication->GetPlayMap(), mPhraseMgr, mMuseSynth);
        mPhrasePlayer->SetJamEffectsMgr(mJamEffects);
    }
}

// 0x001c4cf0
void PitchingSTG::Start() {
    ScoreTrackGraph::Start();
    mPitcher->Start(kMBTInfinity); // Unguarded, as in the binary, for a track of any other kind.
}

// 0x001c4d28
void PitchingSTG::Stop() {
    mPitcher->Stop();
    ScoreTrackGraph::Stop();
}

// 0x001c4c68
PitchingSTG::~PitchingSTG() {
    PitchingSTG::Stop(); // The binary calls this class's body rather than dispatching.
    delete mPitcher;
    delete mJamEffects;
}

// 0x001c4798
void PitchingSTG::ConnectSources(MsgSource *pPrimary, MsgSource *pOptional, MsgSource *pSecondary) {
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
void PitchingSTG::SetMixerOutput(MsgSink *pOutput) {
    mMuseSynth->AddSink(mMixer);
    mMixer->mOutput = pOutput;
}

// 0x001c4da0
void PitchingSTG::AddSinkToSources(MsgSink *pSink) {
    mPhraseMgr->AddSink(pSink);
    mPitcher->AddSink(pSink);
    if (mJamEffects != nullptr) {
        mJamEffects->AddSink(pSink);
    }
}

// 0x001c4e30
void PitchingSTG::SetNetSink(MsgSink *pSink) {
    if (pSink != nullptr) {
        mPhraseMgr->mNetSink = pSink;
    }
}
