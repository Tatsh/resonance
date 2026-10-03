#include "sch/barsequencer.h"

#include <vector>

#include "game/trackdata.h"
#include "mid/mbt.h"

namespace {

// MIDI ticks in one bar, which is also the task's period.
constexpr int kTicksPerBar = 1920;

// The TickTask constructor's bAligned argument.
constexpr int kTickTaskUnaligned = 0;

// The value Tick() returns to keep the task running.
constexpr int kKeepRunning = 1;

} // namespace

// NTSC-U/C: 0x00100c70, PAL: 0x00100c70
BarSequencer::BarSequencer(Sch::TickClock *pClock, TrackData *pTrack, MsgSink *pSink, int nUnmapped)
    : TickTask(pClock, Mid::MBT(kTicksPerBar).mTick, kTickTaskUnaligned), mTrack(pTrack),
      mSink(pSink), mClock(pClock), mUnmapped(nUnmapped), mSequencer(nullptr) {
}

// NTSC-U/C: 0x00100d78, PAL: 0x00100d78
BarSequencer::~BarSequencer() {
    delete mSequencer;
}

// NTSC-U/C: 0x00100c08, PAL: 0x00100c08
void *BarSequencer::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, "BarSequencer");
}

// NTSC-U/C: 0x00100c28, PAL: 0x00100c28
void BarSequencer::operator delete(void *pBlock) {
    FreeTaggedMemory(pBlock, "BarSequencer");
}

// NTSC-U/C: 0x00100c48, PAL: 0x00100c48
void BarSequencer::Print(std::ostream &stream) {
    stream << "{BarSequencer}";
}

// NTSC-U/C: 0x00100370, PAL: 0x00100370
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
