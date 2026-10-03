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

} // namespace

// NTSC-U/C: 0x001b8cc8, PAL: 0x001beaa0
void *PhraseDatabase::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, "PhraseDatabase");
}

// NTSC-U/C: 0x001b8ce8, PAL: 0x001beac0
void PhraseDatabase::operator delete(void *pBlock) {
    OperatorDeleteOverride(pBlock, "PhraseDatabase");
}

// NTSC-U/C: 0x001b72d8, PAL: 0x001bd0b0
PhraseDatabase::PhraseDatabase(PlayMap *pMap) : mMap(pMap) {
    mPhrases.resize(pMap->mSteps.back());
    mStepEffects.resize(pMap->mSteps.size() - 1);
}

// NTSC-U/C: 0x001b75e8, PAL: 0x001bd3c0
PhraseDatabase::~PhraseDatabase() {
    Clear();
}

// NTSC-U/C: 0x001b77a0, PAL: 0x001bd578
void PhraseDatabase::SetOwners(Player *pPlayer) {
    const int nCount = mPhrases.size();
    for (int i = 0; i < nCount; ++i) {
        if (mPhrases[i] == nullptr) {
            mPhrases[i] = new Phrase;
        }
        mPhrases[i]->mPlayer = pPlayer;
    }
}

// NTSC-U/C: 0x001b7898, PAL: 0x001bd670
void PhraseDatabase::Save(OBStream &stream) {
    const char cVersion = kSaveVersion;
    stream.Write(&cVersion, sizeof(cVersion));

    const int nPhraseCount = mPhrases.size();
    stream.WriteLE(&nPhraseCount, sizeof(nPhraseCount));
    for (unsigned i = 0; i < mPhrases.size(); ++i) {
        stream << mPhrases[i];
    }

    const int nValueCount = mStepEffects.size();
    stream.WriteLE(&nValueCount, sizeof(nValueCount));
    for (unsigned i = 0; i < mStepEffects.size(); ++i) {
        // The unsigned long overload writes the low word.
        stream << static_cast<unsigned long>(mStepEffects[i]);
    }
}

// NTSC-U/C: 0x001b7a28, PAL: 0x001bd800
void PhraseDatabase::Load(IBStream &stream) {
    Clear();

    char cVersion;
    stream.Read(&cVersion, sizeof(cVersion));

    int nCount;
    stream.ReadLE(&nCount, sizeof(nCount));
    for (int i = 0; i < nCount; ++i) {
        stream >> mPhrases[i];
    }

    stream.ReadLE(&nCount, sizeof(nCount));
    for (int i = 0; i < nCount; ++i) {
        int nEffects;
        stream.ReadLE(&nEffects, sizeof(nEffects));
        mStepEffects[i] = nEffects;
    }
}

// NTSC-U/C: 0x001b8d60, PAL: 0x001beb38
void PhraseDatabase::Clear() {
    std::for_each(mPhrases.begin(), mPhrases.end(), Attachment::ReleaseIfSet);
    std::fill(mPhrases.begin(), mPhrases.end(), static_cast<Phrase *>(nullptr));
    std::fill(mStepEffects.begin(), mStepEffects.end(), 0LL);
}

// NTSC-U/C: 0x001b8dc8, PAL: 0x001beba0
void PhraseDatabase::ClearOwners() {
    std::cout << "PhraseDatabase::ClearOwners\n";
    for (std::vector<Phrase *>::iterator it = mPhrases.begin(); it != mPhrases.end(); ++it) {
        if (*it != nullptr) {
            (*it)->mPlayer = &NullPlayer::sInstance;
        }
    }
    std::cout << "PhraseDatabase::ClearOwners done\n";
}

// NTSC-U/C: 0x001b8e50, PAL: 0x001bec28
Phrase *PhraseDatabase::GetPhraseAt(int nBar) {
    return mPhrases[mMap->MapBar(nBar)];
}

// NTSC-U/C: 0x001b8e98, PAL: 0x001bec70
Phrase *PhraseDatabase::GetPhrase(int nIndex) {
    return mPhrases[nIndex];
}

// NTSC-U/C: 0x001b8eb0, PAL: 0x001bec88
void PhraseDatabase::SetPhraseAt(Phrase *pPhrase, int nTick) {
    SetPhrase(pPhrase, mMap->MapBar(nTick));
}

// NTSC-U/C: 0x001b8f08, PAL: 0x001bece0
void PhraseDatabase::SetPhrase(Phrase *pPhrase, int nIndex) {
    if (mPhrases[nIndex] != nullptr) {
        mPhrases[nIndex]->Release();
    }
    mPhrases[nIndex] = pPhrase;
    if (pPhrase != nullptr) {
        ++pPhrase->mRefs;
    }
}

// NTSC-U/C: 0x001b8f78, PAL: 0x001bed50
void PhraseDatabase::ClearPhraseAt(int nTick) {
    ClearPhrase(mMap->MapBar(nTick));
}

// NTSC-U/C: 0x001b8fc0, PAL: 0x001bed98
void PhraseDatabase::ClearPhrase(int nIndex) {
    if (mPhrases[nIndex] != nullptr) {
        mPhrases[nIndex]->Release();
    }
    mPhrases[nIndex] = nullptr;
}

// NTSC-U/C: 0x001b9018, PAL: 0x001bedf0
void PhraseDatabase::SetOwnerAt(Player *pPlayer, int nTick) {
    SetOwner(pPlayer, mMap->MapBar(nTick));
}

// NTSC-U/C: 0x001b9070, PAL: 0x001bee48
void PhraseDatabase::SetOwner(Player *pPlayer, int nIndex) {
    if (mPhrases[nIndex] == nullptr) {
        mPhrases[nIndex] = new Phrase;
    }
    mPhrases[nIndex]->mPlayer = pPlayer;
}

// NTSC-U/C: 0x001b9130, PAL: 0x001bef08
Player *PhraseDatabase::GetOwner(int nIndex) {
    Phrase *pPhrase = mPhrases[nIndex];
    if (pPhrase == nullptr) {
        return &NullPlayer::sInstance;
    }
    return pPhrase->mPlayer;
}

// NTSC-U/C: 0x001b9158, PAL: 0x001bef30
void PhraseDatabase::SetPhraseByte(int nIndex, char cValue) {
    Phrase *pPhrase = mPhrases[nIndex];
    if (pPhrase != nullptr) {
        pPhrase->mScore = cValue;
    }
}

// NTSC-U/C: 0x001b9178, PAL: 0x001bef50
unsigned char PhraseDatabase::GetPhraseByte(int nIndex) {
    Phrase *pPhrase = mPhrases[nIndex];
    if (pPhrase == nullptr) {
        return 0;
    }
    return pPhrase->mScore;
}

// NTSC-U/C: 0x001b91a0, PAL: 0x001bef78
long long *PhraseDatabase::GetStepValue(int nBar) {
    return &mStepEffects[mMap->FindStepIndex(mMap->MapBar(nBar))];
}

// NTSC-U/C: 0x001b91f0, PAL: 0x001befc8
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
