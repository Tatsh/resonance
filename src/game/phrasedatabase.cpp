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

void *PhraseDatabase::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, "PhraseDatabase");
}

void PhraseDatabase::operator delete(void *pBlock) {
    OperatorDeleteOverride(pBlock, "PhraseDatabase");
}

PhraseDatabase::PhraseDatabase(PlayMap *pMap) : mMap(pMap) {
    mPhrases.resize(pMap->mSteps.back());
    mStepEffects.resize(pMap->mSteps.size() - 1);
}

PhraseDatabase::~PhraseDatabase() {
    Clear();
}

void PhraseDatabase::SetOwners(Player *pPlayer) {
    const int nCount = mPhrases.size();
    for (int i = 0; i < nCount; ++i) {
        if (mPhrases[i] == nullptr) {
            mPhrases[i] = new Phrase;
        }
        mPhrases[i]->mPlayer = pPlayer;
    }
}

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

void PhraseDatabase::Clear() {
    std::for_each(mPhrases.begin(), mPhrases.end(), Attachment::ReleaseIfSet);
    std::fill(mPhrases.begin(), mPhrases.end(), static_cast<Phrase *>(nullptr));
    std::fill(mStepEffects.begin(), mStepEffects.end(), 0LL);
}

void PhraseDatabase::ClearOwners() {
    std::cout << "PhraseDatabase::ClearOwners\n";
    for (std::vector<Phrase *>::iterator it = mPhrases.begin(); it != mPhrases.end(); ++it) {
        if (*it != nullptr) {
            (*it)->mPlayer = &NullPlayer::sInstance;
        }
    }
    std::cout << "PhraseDatabase::ClearOwners done\n";
}

Phrase *PhraseDatabase::GetPhraseAt(int nBar) {
    return mPhrases[mMap->MapBar(nBar)];
}

Phrase *PhraseDatabase::GetPhrase(int nIndex) {
    return mPhrases[nIndex];
}

void PhraseDatabase::SetPhraseAt(Phrase *pPhrase, int nTick) {
    SetPhrase(pPhrase, mMap->MapBar(nTick));
}

void PhraseDatabase::SetPhrase(Phrase *pPhrase, int nIndex) {
    if (mPhrases[nIndex] != nullptr) {
        mPhrases[nIndex]->Release();
    }
    mPhrases[nIndex] = pPhrase;
    if (pPhrase != nullptr) {
        ++pPhrase->mRefs;
    }
}

void PhraseDatabase::ClearPhraseAt(int nTick) {
    ClearPhrase(mMap->MapBar(nTick));
}

void PhraseDatabase::ClearPhrase(int nIndex) {
    if (mPhrases[nIndex] != nullptr) {
        mPhrases[nIndex]->Release();
    }
    mPhrases[nIndex] = nullptr;
}

void PhraseDatabase::SetOwnerAt(Player *pPlayer, int nTick) {
    SetOwner(pPlayer, mMap->MapBar(nTick));
}

void PhraseDatabase::SetOwner(Player *pPlayer, int nIndex) {
    if (mPhrases[nIndex] == nullptr) {
        mPhrases[nIndex] = new Phrase;
    }
    mPhrases[nIndex]->mPlayer = pPlayer;
}

Player *PhraseDatabase::GetOwner(int nIndex) {
    Phrase *pPhrase = mPhrases[nIndex];
    if (pPhrase == nullptr) {
        return &NullPlayer::sInstance;
    }
    return pPhrase->mPlayer;
}

void PhraseDatabase::SetPhraseByte(int nIndex, char cValue) {
    Phrase *pPhrase = mPhrases[nIndex];
    if (pPhrase != nullptr) {
        pPhrase->mScore = cValue;
    }
}

unsigned char PhraseDatabase::GetPhraseByte(int nIndex) {
    Phrase *pPhrase = mPhrases[nIndex];
    if (pPhrase == nullptr) {
        return 0;
    }
    return pPhrase->mScore;
}

long long *PhraseDatabase::GetStepValue(int nBar) {
    return &mStepEffects[mMap->FindStepIndex(mMap->MapBar(nBar))];
}

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
