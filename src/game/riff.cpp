#include "game/riff.h"

#include <algorithm>
#include <cstddef>
#include <iostream>

#include "mid/mbt.h"
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
    if (nTick <= Mid::MBT(kSnapToStartTicks).mTick) {
        return Mid::MBT(kRiffStartTick).mTick;
    }
    if (Mid::MBT(kLateTicks).mTick < nTick) {
        const int nShifted = nTick + Mid::MBT(kLateShiftTicks).mTick;
        return std::min(kMBTMaximum, std::max(kMBTMinimum, nShifted));
    }
    return nTick;
}

} // namespace

// 0x001ceb70
void *Riff::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, "Riff");
}

// 0x001ceb90
void Riff::operator delete(void *pBlock) {
    FreeTaggedMemory(pBlock, "Riff");
}

// 0x001cebb0
Riff::Riff(int nId) : mLength(0) {
    mId = nId;
}

// 0x001cec58
void Riff::Print(std::ostream &stream) {
    stream << "riff[id=" << mId << "]";
    MultiMuse::Print(stream);
}

// 0x001ce778
void Riff::AddNoteMsg(
    int nTick, unsigned char nNote, unsigned char nVelocity, int nLength, unsigned char nChannel) {
    const int nAdjusted = AdjustRiffTick(nTick);
    // The length is stored without the finite check Mid::MBT(int) makes.
    Mid::MBT length;
    length.mTick = nLength;
    NoteMsg msg(kMBTInfinity, nChannel, nNote, nVelocity, length);
    Add(&msg, nAdjusted, kCheckLast);
}

// 0x001ce8d0
void Riff::AddMidiMsg(int nTick,
                      unsigned char nStatus,
                      unsigned char nData1,
                      unsigned char nData2,
                      unsigned char nChannel) {
    const int nAdjusted = AdjustRiffTick(nTick);
    StdMidiMsg msg(kMBTInfinity, nStatus | nChannel, nData1, nData2);
    Add(&msg, nAdjusted, kCheckLast);
}
