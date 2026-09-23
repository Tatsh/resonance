#include "gs/multimuse.h"

#include <algorithm>
#include <cstddef>
#include <vector>

#include "game/tickobjvector.h"
#include "gs/nlfilebuf.h"
#include "msg/messageio.h"
#include "os/mem.h"

namespace {

// The indent Print() starts from, meaning the stream has no nlfilebuf to measure a column with.
constexpr long long kNoIndent = -1;

} // namespace

// 0x001a8448
MultiMuse::~MultiMuse() {
    for (std::vector<TickObj<MuseMsg *> >::iterator it = mEntries.begin(); it != mEntries.end();
         ++it) {
        delete it->mValue;
    }
}

// 0x001a8580
void MultiMuse::Print(std::ostream &stream) {
    long long nIndent = kNoIndent;
    std::vector<TickObj<MuseMsg *> >::iterator it = mEntries.begin();
    if (it == mEntries.end()) {
        stream << "[empty]";
        return;
    }

    std::ostream &open = stream << "[";
    nlfilebuf *pBuffer = dynamic_cast<nlfilebuf *>(open.rdbuf());
    if (pBuffer != nullptr) {
        nIndent = static_cast<int>(open.tellp()) - pBuffer->mLineStart;
    }
    PrintMuseEntry(open, it->mPosition, it->mValue);

    for (++it; it != mEntries.end(); ++it) {
        std::ostream &line = stream << std::endl;
        if (nIndent != kNoIndent) {
            nlfilebuf *pLineBuffer = dynamic_cast<nlfilebuf *>(line.rdbuf());
            if (pLineBuffer != nullptr) {
                const long long nPad =
                    nIndent - (static_cast<int>(line.tellp()) - pLineBuffer->mLineStart);
                for (int i = 0; i < nPad; ++i) {
                    line << " ";
                }
            }
        }
        pBuffer = dynamic_cast<nlfilebuf *>(stream.rdbuf());
        if (pBuffer != nullptr) {
            nIndent = static_cast<int>(stream.tellp()) - pBuffer->mLineStart;
        }
        PrintMuseEntry(stream, it->mPosition, it->mValue);
    }
    stream << "]";
}

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

// 0x001a97e8
std::ostream &PrintMuseEntry(std::ostream &stream, Mid::MBT position, MuseMsg *pMsg) {
    std::ostream &open = stream << "[";
    position.Print(open);
    std::ostream &separated = open << ": ";
    pMsg->PrintBraced(separated); // Yes, the binary discards the result.
    return separated << "]";
}

// 0x001a8808
void MultiMuse::Append(const MultiMuse &other) {
    mEntries.reserve(other.mEntries.size());
    for (std::vector<TickObj<MuseMsg *> >::const_iterator it = other.mEntries.begin();
         it != other.mEntries.end();
         ++it) {
        TickObj<MuseMsg *> entry;
        entry.mValue = static_cast<MuseMsg *>(it->mValue->Clone());
        entry.mPosition = it->mPosition;
        mEntries.push_back(entry);
    }
}

// 0x001a8a88
void MultiMuse::LoadFields(IBStream &stream) {
    mEntries.clear();
    int nCount;
    stream.Read(&nCount, sizeof(nCount));
    mEntries.reserve(nCount);
    for (int i = 0; i < nCount; ++i) {
        Mid::MBT position;
        position.Load(stream);
        Message *pMsg;
        ReadMessagePointerFromStream(stream, pMsg);
        TickObj<MuseMsg *> entry;
        entry.mPosition = position;
        entry.mValue = dynamic_cast<MuseMsg *>(pMsg);
        mEntries.push_back(entry);
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
