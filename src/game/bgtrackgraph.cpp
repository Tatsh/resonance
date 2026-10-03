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

// NTSC-U/C: 0x0013fb10, PAL: 0x001404f0
BGTrackGraph::BGTrackGraph(int nTrack, int nUnmapped)
    : mSequencer(nullptr), mUnsetLowByte(kUnsetByte), mUnsetHighByte(kUnsetByte), mTrack(nTrack),
      mTrackData(nullptr), mUnmapped(nUnmapped), mMuseSynth(nullptr), mMixer(nullptr) {
    mMuseSynth = new MuseSynth(Application::shared()->GetSongClock());
    mDisabler = new MidiDisabler(kDisablerStartsEnabled);
    mDisabler->AddSink(mMuseSynth);
}

// NTSC-U/C: 0x00140338, PAL: 0x00140d18
BGTrackGraph::~BGTrackGraph() {
    Stop();
    delete mDisabler;
    delete mMuseSynth;
    delete mMixer;
}

// NTSC-U/C: 0x0013fc48, PAL: 0x00140628
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

// NTSC-U/C: 0x001403e0, PAL: 0x00140dc0
void BGTrackGraph::CreateMixer(TrackData *pTrack) {
    mTrackData = pTrack;
    mMixer = new Mixer(kNoMixerTrack, pTrack->mChannel);
}

// NTSC-U/C: 0x00140468, PAL: 0x00140e48
void BGTrackGraph::AttachMixerToSource(MsgSource *pSource) {
    pSource->AddSink(mMixer);
}

// NTSC-U/C: 0x00140498, PAL: 0x00140e78
void BGTrackGraph::AttachMixerToSynth(MsgSink *pSynth) {
    mMuseSynth->AddSink(mMixer);
    mMixer->mOutput = pSynth;
}

// NTSC-U/C: 0x001404d8, PAL: 0x00140eb8
void BGTrackGraph::AddSynthSink(MsgSink *pSink) {
    mMuseSynth->AddSink(pSink);
}

// NTSC-U/C: 0x001404f8, PAL: 0x00140ed8
void BGTrackGraph::Stop() {
    delete mSequencer;
    mSequencer = nullptr;
}

// NTSC-U/C: 0x00140540, PAL: 0x00140f20
void BGTrackGraph::EnableMidi() {
    mDisabler->Enable();
}

// NTSC-U/C: 0x00140560, PAL: 0x00140f40
void BGTrackGraph::DisableMidi() {
    mDisabler->Disable();
}
