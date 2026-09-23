#include "game/axingstg.h"

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "mid/mbt.h"
#include "synth/ps2hardsynth.h"

namespace {

// The play mode in which the stage announces itself over MIDI.
constexpr int kPlayModeAnnouncing = 2;

// MIDI status nibble for a control change, combined with the track's channel.
constexpr unsigned char kMidiControlChange = 0xb0;

// The controller the stage raises while it is running.
constexpr unsigned char kStageActiveController = 0x52;

constexpr unsigned char kControllerOn = 0x7f;
constexpr unsigned char kControllerOff = 0;

// The period of the phrase maker's periodical post, one bar.
constexpr int kBarTicks = 1920;

} // namespace

// 0x0019daf0
AxingSTG::AxingSTG(const TrackData *pTrackData) : ScoreTrackGraph(pTrackData) {
    mMuseSynth->CreateSustainer();

    mAutoRiffer = new AutoRiffer(mApplication->GetSongClock(), mQuantizer, mTrackData);
    mPitchPicker = new PitchPicker(mTrackData);
    mAxisControl = new AxisControl(mTrackData);
    mOldGemMaker = new AxeOldGemMaker(mTrackData);
    mNewGemMaker = new AxeNewGemMaker(mTrackData);
    mSustainer = new SynthSustainer();
    mPhraseMaker =
        new AxePhraseMaker(mPhraseMgr, mQuantizer, mTrackData, mApplication->GetSongClock());
    mAxeSynth = new MuseSynth(mApplication->GetSongClock());
    mPeriodical =
        new GsPeriodical(mApplication->GetSongClock(), mPhraseMaker, Mid::MBT(kBarTicks).mTick);
}

// 0x0019de08
AxingSTG::~AxingSTG() {
    AxingSTG::Slot3(); // The binary calls this class's own body rather than dispatching.
    delete mPeriodical;
    delete mAxeSynth;
    delete mPhraseMaker;
    delete mSustainer;
    delete mNewGemMaker;
    delete mOldGemMaker;
    delete mAxisControl;
    delete mPitchPicker;
    delete mAutoRiffer;
}

// 0x0019e720
void AxingSTG::Slot2() {
    if (mApplication->GetGameManager()->GetPlayMode() == kPlayModeAnnouncing) {
        const unsigned char nStatus =
            static_cast<unsigned char>(kMidiControlChange | mTrackData->mChannel);
        mApplication->GetSynth()->SendMidi(nStatus, kStageActiveController, kControllerOn);
    }
    ScoreTrackGraph::Slot2();
    mPeriodical->Post();
}

// 0x0019e798
void AxingSTG::Slot3() {
    if (mApplication->GetGameManager()->GetPlayMode() == kPlayModeAnnouncing) {
        const unsigned char nStatus =
            static_cast<unsigned char>(kMidiControlChange | mTrackData->mChannel);
        mApplication->GetSynth()->SendMidi(nStatus, kStageActiveController, kControllerOff);
    }
    mPeriodical->Withdraw();
    ScoreTrackGraph::Slot3();
}

// 0x0019df58
void AxingSTG::Slot4(MsgSource *pPrimary, MsgSource *pOptional, MsgSource *pSecondary) {
    pPrimary->AddSink(mMixer);
    pPrimary->AddSink(mPhraseMaker);
    pPrimary->AddSink(mNewGemMaker);
    pPrimary->AddSink(mAutoRiffer);
    pPrimary->AddSink(mAxisControl);
    pPrimary->AddSink(mPitchPicker);
    pPrimary->AddSink(mPhraseMgr);
    pPrimary->AddSink(mPhrasePlayer);

    mAutoRiffer->mSource.AddSink(mPitchPicker);
    mAutoRiffer->mSource.AddSink(mAxeSynth);
    mAutoRiffer->mSynth = mMuseSynth;
    mAutoRiffer->mPhraseMaker = mPhraseMaker;

    mAxeSynth->AddSink(mPitchPicker);

    mPitchPicker->AddSink(mPhraseMaker);
    mPitchPicker->AddSink(mNewGemMaker);
    mPitchPicker->AddSink(mMuseSynth);
    mPitchPicker->AddSink(mAxisControl);

    mAxisControl->AddSink(mAxeSynth);
    mAxisControl->AddSink(mPhraseMaker);

    mPhraseMgr->AddSink(mOldGemMaker);

    pSecondary->AddSink(mPhraseMgr);
    pSecondary->AddSink(mPhraseMaker);

    if (pOptional != nullptr) {
        pOptional->AddSink(mPhraseMgr);
    }

    mPhrasePlayer->AddSink(mMuseSynth);
}

// 0x0019e810
void AxingSTG::Slot6(MsgSink *pOutput) {
    mMuseSynth->AddSink(mMixer);
    mMixer->mOutput = pOutput;
}

// 0x0019e850
void AxingSTG::Slot7(MsgSink *pSink) {
    mPhraseMgr->AddSink(pSink);
    mPhraseMaker->AddSink(pSink);
    mOldGemMaker->AddSink(pSink);
    mNewGemMaker->AddSink(pSink);
    mAxisControl->AddSink(pSink);
    mAutoRiffer->mSource.AddSink(pSink);
}

// 0x0019e928
void AxingSTG::Slot8(MsgSink *pSink) {
    if (pSink != nullptr) {
        mPhraseMgr->mNetSink = pSink;
    }
}
