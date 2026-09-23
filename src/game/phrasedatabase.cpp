#include "game/phrasedatabase.h"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <vector>

#include "game/nullplayer.h"
#include "game/phrase.h"
#include "game/playmap.h"
#include "os/mem.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

namespace {

constexpr char kSaveVersion = 1;

// Print() starts a new line before every eighth phrase.
constexpr unsigned kPhrasesPerLine = 8;

// 0x001b8d40
void ReleasePhrase(Phrase *pPhrase) {
    if (pPhrase != nullptr) {
        pPhrase->Release();
    }
}

} // namespace

// 0x001b8cc8
void *PhraseDatabase::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, "PhraseDatabase");
}

// 0x001b8ce8
void PhraseDatabase::operator delete(void *pBlock) {
    FreeTaggedMemory(pBlock, "PhraseDatabase");
}

// 0x001b72d8
PhraseDatabase::PhraseDatabase(PlayMap *pMap) : mMap(pMap) {
    mPhrases.resize(pMap->mSteps.back());
    mUnknown20.resize(pMap->mSteps.size() - 1);
}

// 0x001b75e8
PhraseDatabase::~PhraseDatabase() {
    Clear();
}

// 0x001b77a0
void PhraseDatabase::SetOwners(Player *pPlayer) {
    const int nCount = mPhrases.size();
    for (int i = 0; i < nCount; ++i) {
        if (mPhrases[i] == nullptr) {
            mPhrases[i] = new Phrase;
        }
        mPhrases[i]->mPlayer = pPlayer;
    }
}

// 0x001b7898
void PhraseDatabase::Save(OBStream &stream) {
    const char cVersion = kSaveVersion;
    stream.WriteBytes(&cVersion, sizeof(cVersion));

    const int nPhraseCount = mPhrases.size();
    stream.Write(&nPhraseCount, sizeof(nPhraseCount));
    for (unsigned i = 0; i < mPhrases.size(); ++i) {
        stream << mPhrases[i];
    }

    const int nValueCount = mUnknown20.size();
    stream.Write(&nValueCount, sizeof(nValueCount));
    for (unsigned i = 0; i < mUnknown20.size(); ++i) {
        stream << mUnknown20[i];
    }
}

// 0x001b7a28
void PhraseDatabase::Load(IBStream &stream) {
    Clear();

    char cVersion;
    stream.ReadBytes(&cVersion, sizeof(cVersion));

    int nCount;
    stream.Read(&nCount, sizeof(nCount));
    for (int i = 0; i < nCount; ++i) {
        stream >> mPhrases[i];
    }

    stream.Read(&nCount, sizeof(nCount));
    for (int i = 0; i < nCount; ++i) {
        int nValue;
        stream.Read(&nValue, sizeof(nValue));
        mUnknown20[i] = nValue;
    }
}

// 0x001b8d60
void PhraseDatabase::Clear() {
    std::for_each(mPhrases.begin(), mPhrases.end(), ReleasePhrase);
    std::fill(mPhrases.begin(), mPhrases.end(), static_cast<Phrase *>(nullptr));
    std::fill(mUnknown20.begin(), mUnknown20.end(), 0L);
}

// 0x001b8dc8
void PhraseDatabase::ClearOwners() {
    std::cout << "PhraseDatabase::ClearOwners\n";
    for (std::vector<Phrase *>::iterator it = mPhrases.begin(); it != mPhrases.end(); ++it) {
        if (*it != nullptr) {
            (*it)->mPlayer = &g_nullPlayer;
        }
    }
    std::cout << "PhraseDatabase::ClearOwners done\n";
}

// 0x001b8e50
Phrase *PhraseDatabase::GetPhraseAt(int nBar) {
    return mPhrases[mMap->Slot5(nBar)];
}

// 0x001b8e98
Phrase *PhraseDatabase::GetPhrase(int nIndex) {
    return mPhrases[nIndex];
}

// 0x001b8eb0
void PhraseDatabase::SetPhraseAt(Phrase *pPhrase, int nTick) {
    SetPhrase(pPhrase, mMap->Slot5(nTick));
}

// 0x001b8f08
void PhraseDatabase::SetPhrase(Phrase *pPhrase, int nIndex) {
    if (mPhrases[nIndex] != nullptr) {
        mPhrases[nIndex]->Release();
    }
    mPhrases[nIndex] = pPhrase;
    if (pPhrase != nullptr) {
        ++pPhrase->mRefs;
    }
}

// 0x001b8f78
void PhraseDatabase::ClearPhraseAt(int nTick) {
    ClearPhrase(mMap->Slot5(nTick));
}

// 0x001b8fc0
void PhraseDatabase::ClearPhrase(int nIndex) {
    if (mPhrases[nIndex] != nullptr) {
        mPhrases[nIndex]->Release();
    }
    mPhrases[nIndex] = nullptr;
}

// 0x001b9018
void PhraseDatabase::SetOwnerAt(Player *pPlayer, int nTick) {
    SetOwner(pPlayer, mMap->Slot5(nTick));
}

// 0x001b9070
void PhraseDatabase::SetOwner(Player *pPlayer, int nIndex) {
    if (mPhrases[nIndex] == nullptr) {
        mPhrases[nIndex] = new Phrase;
    }
    mPhrases[nIndex]->mPlayer = pPlayer;
}

// 0x001b9130
Player *PhraseDatabase::GetOwner(int nIndex) {
    Phrase *pPhrase = mPhrases[nIndex];
    if (pPhrase == nullptr) {
        return &g_nullPlayer;
    }
    return pPhrase->mPlayer;
}

// 0x001b9158
void PhraseDatabase::SetPhraseByte(int nIndex, char cValue) {
    Phrase *pPhrase = mPhrases[nIndex];
    if (pPhrase != nullptr) {
        pPhrase->mUnknown28 = cValue;
    }
}

// 0x001b9178
unsigned char PhraseDatabase::GetPhraseByte(int nIndex) {
    Phrase *pPhrase = mPhrases[nIndex];
    if (pPhrase == nullptr) {
        return 0;
    }
    return pPhrase->mUnknown28;
}

// 0x001b91a0
long *PhraseDatabase::GetStepValue(int nBar) {
    return &mUnknown20[mMap->FindStepIndex(mMap->Slot5(nBar))];
}

// 0x001b91f0
void PhraseDatabase::Print(std::ostream &stream) {
    for (unsigned i = 0; i < mPhrases.size(); ++i) {
        if (i != 0 && (i & (kPhrasesPerLine - 1)) == 0) {
            stream << std::endl;
        }
        if (mPhrases[i] != nullptr) {
            stream << *mPhrases[i] << " ";
        } else {
            stream << "[null] ";
        }
    }
    stream << std::endl;
}
