#include "game/riff.h"

#include <algorithm>
#include <cstddef>
#include <iostream>

#include "mid/tick.h"
#include "msg/notemsg.h"
#include "msg/stdmidimsg.h"
#include "os/mem.h"

namespace {

// A position at or before this tick moves to the start of the riff.
constexpr int kSnapToStartTicks = 2;
constexpr int kRiffStartTick = 0;

// A position after this tick moves this many ticks later.
constexpr int kLateTicks = 30;
constexpr int kLateShiftTicks = 1;

// MultiMuse::Add() is allowed to append after the last message.
constexpr int kCheckLast = 1;

// The position both adders schedule at, after the adjustment above.
inline int AdjustRiffTick(int nTick) {
    if (nTick <= Sch::Tick(kSnapToStartTicks).mTick) {
        return Sch::Tick(kRiffStartTick).mTick;
    }
    if (Sch::Tick(kLateTicks).mTick < nTick) {
        const int nShifted = nTick + Sch::Tick(kLateShiftTicks).mTick;
        return std::min(kTickMaximum, std::max(kTickMinimum, nShifted));
    }
    return nTick;
}

} // namespace

void *Riff::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, "Riff");
}

void Riff::operator delete(void *pBlock) {
    OperatorDeleteOverride(pBlock, "Riff");
}

Riff::Riff(int nId) : mLength(0) {
    mId = nId;
}

void Riff::Print(std::ostream &stream) {
    stream << "riff[id=" << mId << "]";
    MultiMuse::Print(stream);
}

void Riff::AddNoteMsg(
    int nTick, unsigned char nNote, unsigned char nVelocity, int nLength, unsigned char nChannel) {
    const int nAdjusted = AdjustRiffTick(nTick);
    // The length is stored without the finite check Sch::Tick(int) makes.
    Sch::Tick length;
    length.mTick = nLength;
    NoteMsg msg(kTickInfinity, nChannel, nNote, nVelocity, length);
    Add(&msg, nAdjusted, kCheckLast);
}

void Riff::AddMidiMsg(int nTick,
                      unsigned char nStatus,
                      unsigned char nData1,
                      unsigned char nData2,
                      unsigned char nChannel) {
    const int nAdjusted = AdjustRiffTick(nTick);
    StdMidiMsg msg(kTickInfinity, nStatus | nChannel, nData1, nData2);
    Add(&msg, nAdjusted, kCheckLast);
}
