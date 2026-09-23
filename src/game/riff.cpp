#include "game/riff.h"

#include <cstddef>
#include <iostream>

#include "mid/mbt.h"
#include "os/mem.h"

// 0x001ceb70
void *Riff::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, "Riff");
}

// 0x001ceb90
void Riff::operator delete(void *pBlock) {
    FreeTaggedMemory(pBlock, "Riff");
}

// 0x001cebb0
Riff::Riff(int nId) {
    mUnknown18.mTick = 0;
    (void)IsFiniteMBT(0); // Yes, the binary discards this call's result.
    mId = nId;
}

// 0x001cec58
void Riff::Print(std::ostream &stream) {
    stream << "riff[id=" << mId << "]";
    MultiMuse::Print(stream);
}
