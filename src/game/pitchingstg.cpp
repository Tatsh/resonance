#include "game/pitchingstg.h"

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "mid/tick.h"

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

void PitchingSTG::Start() {
    ScoreTrackGraph::Start();
    mPitcher->Start(kTickInfinity); // Unguarded, as in the binary, for a track of any other kind.
}

void PitchingSTG::Stop() {
    mPitcher->Stop();
    ScoreTrackGraph::Stop();
}

PitchingSTG::~PitchingSTG() {
    PitchingSTG::Stop(); // The binary calls this class's body rather than dispatching.
    delete mPitcher;
    delete mJamEffects;
}

void PitchingSTG::ConnectInputs(MsgSource *pPrimary, MsgSource *pOptional, MsgSource *pSecondary) {
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

void PitchingSTG::SetMixerOutput(MsgSink *pOutput) {
    mMuseSynth->AddSink(mMixer);
    mMixer->mOutput = pOutput;
}

void PitchingSTG::ConnectToTunnel(MsgSink *pSink) {
    mPhraseMgr->AddSink(pSink);
    mPitcher->AddSink(pSink);
    if (mJamEffects != nullptr) {
        mJamEffects->AddSink(pSink);
    }
}

void PitchingSTG::SetNetSink(MsgSink *pSink) {
    if (pSink != nullptr) {
        mPhraseMgr->mNetSink = pSink;
    }
}
