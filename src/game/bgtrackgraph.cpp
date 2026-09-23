#include "game/bgtrackgraph.h"

#include <vector>

#include "app/application.h"
#include "game/middisabler.h"
#include "game/midichase.h"
#include "game/playmap.h"
#include "game/trackdata.h"
#include "gs/mixer.h"
#include "gs/musesynth.h"
#include "mid/mbt.h"
#include "msg/musemsg.h"
#include "os/mem.h"
#include "sch/barsequencer.h"

namespace {

constexpr char kBGTrackGraphTag[] = "BGTrackGraph";

// The value both bytes at +0x04 start at.
constexpr unsigned char kUnsetByte = 0xff;

// The filter starts passing notes.
constexpr int kDisablerStartsEnabled = 1;

// The track a background mixer is built for, which is none.
constexpr int kNoMixerTrack = -1;

} // namespace

void *BGTrackGraph::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, kBGTrackGraphTag);
}

void BGTrackGraph::operator delete(void *pBlock) {
    FreeTaggedMemory(pBlock, kBGTrackGraphTag);
}

// 0x0013fb10
BGTrackGraph::BGTrackGraph(int nTrack, int nUnmapped)
    : mSequencer(nullptr), mUnknown04(kUnsetByte), mUnknown05(kUnsetByte), mTrack(nTrack),
      mTrackData(nullptr), mUnmapped(nUnmapped), mMuseSynth(nullptr), mMixer(nullptr) {
    mMuseSynth = new MuseSynth(Application::shared()->GetSongClock());
    mDisabler = new MidiDisabler(kDisablerStartsEnabled);
    mDisabler->AddSink(mMuseSynth);
}

// 0x00140338
BGTrackGraph::~BGTrackGraph() {
    DeleteSequencer();
    delete mDisabler;
    delete mMuseSynth;
    delete mMixer;
}

// 0x0013fc48
void BGTrackGraph::BuildSequencer() {
    MidiChase chase;
    const int nBarCount = Application::shared()->GetPlayMap()->mBarCount;
    for (int i = 0; i < nBarCount; ++i) {
        const std::vector<TickObj<MuseMsg *> > *pMidi = mTrackData->GetMidiInBar(i);
        chase.HandleRange(pMidi->data(), pMidi->data() + pMidi->size());
    }
    chase.Replay(mMuseSynth);

    mSequencer =
        new BarSequencer(Application::shared()->GetSongClock(), mTrackData, mDisabler, mUnmapped);
    mSequencer->Start(Mid::MBT(0).mTick);
}

// 0x001403e0
void BGTrackGraph::CreateMixer(TrackData *pTrack) {
    mTrackData = pTrack;
    mMixer = new Mixer(kNoMixerTrack, pTrack->mChannel);
}

// 0x00140468
void BGTrackGraph::AttachMixerToSource(MsgSource *pSource) {
    pSource->AddSink(mMixer);
}

// 0x00140498
void BGTrackGraph::AttachMixerToSynth(MsgSink *pSynth) {
    mMuseSynth->AddSink(mMixer);
    mMixer->mOutput = pSynth;
}

// 0x001404d8
void BGTrackGraph::AddSynthSink(MsgSink *pSink) {
    mMuseSynth->AddSink(pSink);
}

// 0x001404f8
void BGTrackGraph::DeleteSequencer() {
    delete mSequencer;
    mSequencer = nullptr;
}

// 0x00140540
void BGTrackGraph::EnableMidi() {
    mDisabler->Enable();
}

// 0x00140560
void BGTrackGraph::DisableMidi() {
    mDisabler->Disable();
}
