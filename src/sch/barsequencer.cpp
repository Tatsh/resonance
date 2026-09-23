#include "sch/barsequencer.h"

#include <vector>

#include "game/trackdata.h"
#include "mid/mbt.h"

namespace {

// MIDI ticks in one bar, which is also the task's period.
constexpr int kTicksPerBar = 1920;

// The third TickTask constructor argument every recovered caller passes.
constexpr int kTickTaskUnknown = 0;

// The value Tick() returns to keep the task running.
constexpr int kKeepRunning = 1;

} // namespace

BarSequencer::BarSequencer(Sch::TickClock *pClock, TrackData *pTrack, MsgSink *pSink, int nUnmapped)
    : TickTask(pClock, Mid::MBT(kTicksPerBar).mTick, kTickTaskUnknown), mTrack(pTrack),
      mSink(pSink), mClock(pClock), mUnmapped(nUnmapped), mSequencer(nullptr) {
}

BarSequencer::~BarSequencer() {
    delete mSequencer;
}

void *BarSequencer::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, "BarSequencer");
}

void BarSequencer::operator delete(void *pBlock) {
    FreeTaggedMemory(pBlock, "BarSequencer");
}

void BarSequencer::Print(std::ostream &stream) {
    stream << "{BarSequencer}";
}

int BarSequencer::Tick(int nTick) {
    const int nBar = nTick / Mid::MBT(kTicksPerBar).mTick;
    const std::vector<TickObj<MuseMsg *> > *pMidi =
        mUnmapped != 0 ? mTrack->GetMidiInBar(nBar) : mTrack->GetMidi(nBar);
    delete mSequencer;

    const TickObj<MuseMsg *> *pBegin = pMidi->data();
    mSequencer = new Sequencer<const TickObj<MuseMsg *> *>(pBegin, pBegin + pMidi->size());
    mSequencer->Post(mClock, mSink);
    return kKeepRunning;
}
