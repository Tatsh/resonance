#include "game/pitchingstg.h"

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "mid/mbt.h"

// NTSC-U/C: 0x001c45b0, PAL: 0x001ca3f8
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

// NTSC-U/C: 0x001c4cf0, PAL: 0x001cab38
void PitchingSTG::Start() {
    ScoreTrackGraph::Start();
    mPitcher->Start(kMBTInfinity); // Unguarded, as in the binary, for a track of any other kind.
}

// NTSC-U/C: 0x001c4d28, PAL: 0x001cab70
void PitchingSTG::Stop() {
    mPitcher->Stop();
    ScoreTrackGraph::Stop();
}

// NTSC-U/C: 0x001c4c68, PAL: 0x001caab0
PitchingSTG::~PitchingSTG() {
    PitchingSTG::Stop(); // The binary calls this class's body rather than dispatching.
    delete mPitcher;
    delete mJamEffects;
}

// NTSC-U/C: 0x001c4798, PAL: 0x001ca5e0
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

// NTSC-U/C: 0x001c4d60, PAL: 0x001caba8
void PitchingSTG::SetMixerOutput(MsgSink *pOutput) {
    mMuseSynth->AddSink(mMixer);
    mMixer->mOutput = pOutput;
}

// NTSC-U/C: 0x001c4da0, PAL: 0x001cabe8
void PitchingSTG::ConnectToTunnel(MsgSink *pSink) {
    mPhraseMgr->AddSink(pSink);
    mPitcher->AddSink(pSink);
    if (mJamEffects != nullptr) {
        mJamEffects->AddSink(pSink);
    }
}

// NTSC-U/C: 0x001c4e30, PAL: 0x001cac78
void PitchingSTG::SetNetSink(MsgSink *pSink) {
    if (pSink != nullptr) {
        mPhraseMgr->mNetSink = pSink;
    }
}
