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

MultiMuse::~MultiMuse() {
    for (std::vector<TickObj<MuseMsg *> >::iterator it = mEntries.begin(); it != mEntries.end();
         ++it) {
        delete it->mValue;
    }
}

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
    PrintMuseMsgTickObj(open, it->mPosition, it->mValue);

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
        PrintMuseMsgTickObj(stream, it->mPosition, it->mValue);
    }
    stream << "]";
}

void *MultiMuse::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, "MultiMuse");
}

void MultiMuse::operator delete(void *pBlock) {
    OperatorDeleteOverride(pBlock, "MultiMuse");
}

void MultiMuse::SaveFields(OBStream &stream) {
    const int nCount = mEntries.end() - mEntries.begin();
    stream.WriteLE(&nCount, sizeof(nCount));

    std::vector<TickObj<MuseMsg *> >::iterator it = mEntries.begin();
    for (; it != mEntries.end(); ++it) {
        Sch::Tick position = it->mPosition;
        position.saveGuts(stream);
        stream << it->mValue;
    }
}

std::ostream &PrintMuseMsgTickObj(std::ostream &stream, Sch::Tick position, MuseMsg *pMsg) {
    std::ostream &open = stream << "[";
    position.Print(open);
    std::ostream &separated = open << ": ";
    pMsg->Print(separated); // Yes, the binary discards the result.
    return separated << "]";
}

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

void MultiMuse::LoadFields(IBStream &stream) {
    mEntries.clear();
    int nCount;
    stream.ReadLE(&nCount, sizeof(nCount));
    mEntries.reserve(nCount);
    for (int i = 0; i < nCount; ++i) {
        Sch::Tick position;
        position.restoreGuts(stream);
        Message *pMsg;
        stream >> pMsg;
        TickObj<MuseMsg *> entry;
        entry.mPosition = position;
        entry.mValue = dynamic_cast<MuseMsg *>(pMsg);
        mEntries.push_back(entry);
    }
}

void MultiMuse::Add(MuseMsg *pMsg, int nTick, int bCheckLast) {
    TickObj<MuseMsg *> entry;
    entry.mPosition.mTick = nTick;
    entry.mValue = static_cast<MuseMsg *>(pMsg->Clone());
    if (bCheckLast != 0) {
        InsertSorted(mEntries, entry);
    } else {
        InsertAtLowerBound(mEntries, entry);
    }
}

MuseMsg *MultiMuse::Find(int nTick) {
    std::vector<TickObj<MuseMsg *> >::iterator it =
        std::lower_bound(mEntries.begin(), mEntries.end(), nTick, TickObjAfter<MuseMsg *>);
    if (it != mEntries.end() && it->mPosition.mTick == nTick) {
        return it->mValue;
    }
    return nullptr;
}
