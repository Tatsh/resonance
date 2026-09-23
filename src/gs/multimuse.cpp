#include "gs/multimuse.h"

#include <algorithm>
#include <cstddef>
#include <vector>

#include "game/tickobjvector.h"
#include "msg/messageio.h"
#include "os/mem.h"

// 0x001a9490
void *MultiMuse::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, "MultiMuse");
}

// 0x001a94b0
void MultiMuse::operator delete(void *pBlock) {
    FreeTaggedMemory(pBlock, "MultiMuse");
}

// 0x001a9738
void MultiMuse::SaveFields(OBStream &stream) {
    const int nCount = mEntries.end() - mEntries.begin();
    stream.Write(&nCount, sizeof(nCount));

    std::vector<TickObj<MuseMsg *> >::iterator it = mEntries.begin();
    for (; it != mEntries.end(); ++it) {
        Mid::MBT position = it->mPosition;
        position.Save(stream);
        WriteMessagePointerToStream(stream, it->mValue);
    }
}

// 0x001a9650
void MultiMuse::Add(MuseMsg *pMsg, int nTick, int bCheckLast) {
    TickObj<MuseMsg *> entry;
    entry.mPosition.mTick = nTick;
    entry.mValue = static_cast<MuseMsg *>(pMsg->Clone());
    if (bCheckLast != 0) {
        InsertSorted(mEntries, entry);
    } else {
        InsertAtUpperBound(mEntries, entry);
    }
}

// 0x001a96c8
MuseMsg *MultiMuse::Find(int nTick) {
    std::vector<TickObj<MuseMsg *> >::iterator it =
        std::lower_bound(mEntries.begin(), mEntries.end(), nTick, TickObjAfter<MuseMsg *>);
    if (it != mEntries.end() && it->mPosition.mTick == nTick) {
        return it->mValue;
    }
    return nullptr;
}
