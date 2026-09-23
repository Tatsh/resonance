#include "game/phrase.h"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <vector>

#include "game/idable.h"
#include "game/nullplayer.h"
#include "game/player.h"
#include "game/tickobjvector.h"
#include "gs/multimuse.h"
#include "mid/mbt.h"
#include "mid/tickobj.h"
#include "msg/musemsg.h"
#include "os/log.h"
#include "os/mem.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

namespace {

constexpr char kSaveVersion = 2;
constexpr char kUnsupportedVersion = 1;
constexpr char kPresent = '1';
constexpr char kAbsent = '0';

// A value with no entry at or before the requested position.
constexpr float kDefaultValue = 0.5f;

// A value crosses the wire as one byte scaled to this range.
constexpr float kValueScale = 255.0f;
constexpr double kValueScaleDouble = 255.0;

// The player identifier that resolves to no player at all.
constexpr int kNoPlayer = -1;

} // namespace

// 0x001b6a28
void *Phrase::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, "Phrase");
}

// 0x001b6a48
void Phrase::operator delete(void *pBlock) {
    FreeTaggedMemory(pBlock, "Phrase");
}

// 0x001b4940
Phrase::Phrase() : mPlayer(&g_nullPlayer), mMuse(nullptr), mUnknown28(0) {
}

// 0x001b4998
Phrase::~Phrase() {
    if (mMuse != nullptr) {
        mMuse->Release();
    }
}

// 0x001b4b30
int Phrase::AddGem(int nTick, int nGem, int nTrans) {
    Gem gem;
    gem.mPosition.mTick = nTick;
    gem.mGem = nGem;
    gem.mTrans = nTrans;

    int nReplaced = -1;
    if (mGems.size() != 0 && mGems.back().mPosition.mTick < nTick) {
        mGems.push_back(gem);
        return nReplaced;
    }

    std::vector<Gem>::iterator it = std::lower_bound(mGems.begin(), mGems.end(), gem);
    if (it != mGems.end() && it->mPosition.mTick == nTick) {
        nReplaced = it->mGem;
        mGems.erase(it);
    }
    mGems.insert(it, gem);
    return nReplaced;
}

// 0x001b4cd8
void Phrase::AddMuseMsg(int nTick, MuseMsg *pMsg) {
    if (mMuse == nullptr) {
        mMuse = new MultiMuse;
    }
    mMuse->Add(pMsg, nTick, 1);
}

// 0x001b4d68
void Phrase::Print(std::ostream &stream) {
    stream << "[";

    if (mGems.size() != 0) {
        std::ostream &gems = stream << "gems: ";
        gems << "(";
        for (std::vector<Gem>::iterator it = mGems.begin(); it != mGems.end(); ++it) {
            gems << *it;
            gems << " ";
        }
        gems << ")";
        gems << "\n";
    }

    if (mMuse != nullptr) {
        mMuse->Print(stream);
        stream << "\n";
    }

    if (mValues.size() != 0) {
        std::ostream &values = stream << "X: ";
        values << "(";
        for (std::vector<TickObj<float> >::iterator it = mValues.begin(); it != mValues.end();
             ++it) {
            std::ostream &entry = values << "[";
            it->mPosition.Print(entry);
            entry << ": " << it->mValue << "]";
            values << " ";
        }
        values << ")";
    }

    std::ostream &player = stream << "pl: ";
    mPlayer->Print(player);
    player << "]\n";
}

// 0x001b5018
void Phrase::Save(OBStream &stream) {
    const char cVersion = kSaveVersion;
    stream.WriteBytes(&cVersion, sizeof(cVersion));

    const int nCount = mGems.size();
    stream.Write(&nCount, sizeof(nCount));
    for (std::vector<Gem>::iterator it = mGems.begin(); it != mGems.end(); ++it) {
        stream << *it;
    }

    const int nPlayer = mPlayer->mId20;
    stream.Write(&nPlayer, sizeof(nPlayer));

    if (mMuse != nullptr) {
        const char cPresent = kPresent;
        mMuse->SaveFields(stream.WriteBytes(&cPresent, sizeof(cPresent)));
        SaveValues(stream);
    } else {
        const char cAbsent = kAbsent;
        stream.WriteBytes(&cAbsent, sizeof(cAbsent));
    }
}

// 0x001b51f8
void Phrase::Load(IBStream &stream) {
    char cVersion;
    stream.ReadBytes(&cVersion, sizeof(cVersion));
    if (cVersion == kUnsupportedVersion) {
        Fatal("need to write support for version 1 phrase gems");
    } else if (cVersion < kSaveVersion) {
        Fatal("Error reading phrase. Unknown version#");
    } else {
        int nCount;
        stream.Read(&nCount, sizeof(nCount));
        mGems.resize(nCount);
        for (std::vector<Gem>::iterator it = mGems.begin(); it != mGems.end(); ++it) {
            stream >> *it;
        }
    }

    // The pointer half of this pair is always null when it is tested. The pair is an inline
    // reference type of the original whose name does not survive.
    Player *pPlayer = nullptr;
    int nPlayer = kNoPlayer;
    stream.Read(&nPlayer, sizeof(nPlayer));
    if (pPlayer == nullptr) {
        if (nPlayer == kNoPlayer) {
            pPlayer = nullptr;
        } else if (nPlayer == kIDableUnregistered) {
            pPlayer = &g_nullPlayer;
        } else {
            pPlayer = IDable<Player>::sObjects[nPlayer];
        }
    }
    mPlayer = pPlayer;
    if (mPlayer == nullptr) {
        mPlayer = IDable<Player>::sObjects[0];
    }

    char cMuse;
    stream.ReadBytes(&cMuse, sizeof(cMuse));
    if (cMuse == kPresent) {
        delete mMuse;
        mMuse = new MultiMuse;
        mMuse->LoadFields(stream);
        LoadValues(stream);
    }
}

// 0x001b5500
void Phrase::SaveValues(OBStream &stream) {
    const short nCount = mValues.end() - mValues.begin();
    stream.Write(&nCount, sizeof(nCount));

    std::vector<TickObj<float> >::iterator itEnd = mValues.end();
    for (std::vector<TickObj<float> >::iterator it = mValues.begin(); it < itEnd; ++it) {
        const short nTick = it->mPosition.mTick;
        const char cValue = static_cast<int>(it->mValue * kValueScale);
        stream.Write(&nTick, sizeof(nTick)).WriteBytes(&cValue, sizeof(cValue));
    }
}

// 0x001b55f8
void Phrase::LoadValues(IBStream &stream) {
    mValues.clear();

    short nCount;
    stream.Read(&nCount, sizeof(nCount));
    mValues.reserve(nCount);

    for (int i = 0; i < nCount; ++i) {
        short nTick;
        unsigned char cValue;
        stream.Read(&nTick, sizeof(nTick)).ReadBytes(&cValue, sizeof(cValue));
        const float flValue = cValue / kValueScaleDouble;

        TickObj<float> value;
        value.mPosition = Mid::MBT(nTick);
        value.mValue = flValue;
        mValues.push_back(value);
    }
}

// 0x001b6d88
void Phrase::AddValue(int nTick, float flValue) {
    TickObj<float> value;
    value.mPosition.mTick = nTick;
    value.mValue = flValue;
    InsertSorted(mValues, value);
}

// 0x001b6db0
float Phrase::GetValue(int nTick) {
    const auto it = FindAtOrBefore(mValues, nTick);
    float flValue = kDefaultValue;
    if (it != mValues.end()) {
        flValue = it->mValue;
    }
    return flValue;
}

// 0x001b6ba0
OBStream &operator<<(OBStream &stream, const Phrase::Gem &gem) {
    const unsigned short nTick = gem.mPosition.mTick;
    const unsigned char cGem = gem.mGem;
    const unsigned char cTrans = gem.mTrans;
    stream.Write(&nTick, sizeof(nTick))
        .WriteBytes(&cGem, sizeof(cGem))
        .WriteBytes(&cTrans, sizeof(cTrans));
    return stream;
}

// 0x001b6c48
IBStream &operator>>(IBStream &stream, Phrase::Gem &gem) {
    unsigned short nTick;
    unsigned char cGem;
    signed char cTrans;
    stream.Read(&nTick, sizeof(nTick));
    stream.ReadBytes(&cGem, sizeof(cGem));
    stream.ReadBytes(&cTrans, sizeof(cTrans));

    gem.mPosition = Mid::MBT(nTick);
    gem.mGem = cGem;
    gem.mTrans = cTrans;
    return stream;
}

// 0x001b6cf8
std::ostream &operator<<(std::ostream &stream, Phrase::Gem &gem) {
    std::ostream &entry = stream << '[';
    gem.mPosition.Print(entry);
    entry << ' ' << gem.mGem << ' ' << gem.mTrans << ']';
    return stream;
}

// 0x001b6e28
OBStream &operator<<(OBStream &stream, Phrase &phrase) {
    const char cPresent = kPresent;
    stream.WriteBytes(&cPresent, sizeof(cPresent));
    phrase.Save(stream);
    return stream;
}

// 0x001b6e88
IBStream &operator>>(IBStream &stream, Phrase &phrase) {
    char cPresent;
    stream.ReadBytes(&cPresent, sizeof(cPresent));
    phrase.Load(stream);
    return stream;
}

// 0x001b6ee0
OBStream &operator<<(OBStream &stream, Phrase *pPhrase) {
    if (pPhrase != nullptr) {
        const char cPresent = kPresent;
        stream.WriteBytes(&cPresent, sizeof(cPresent));
        pPhrase->Save(stream);
    } else {
        const char cAbsent = kAbsent;
        stream.WriteBytes(&cAbsent, sizeof(cAbsent));
    }
    return stream;
}

// 0x001b6f70
IBStream &operator>>(IBStream &stream, Phrase *&pPhrase) {
    char cPresent;
    stream.ReadBytes(&cPresent, sizeof(cPresent));
    if (cPresent == kAbsent) {
        pPhrase = nullptr;
    } else {
        pPhrase = new Phrase;
        pPhrase->Load(stream);
    }
    return stream;
}

// 0x001b7038
std::ostream &operator<<(std::ostream &stream, Phrase &phrase) {
    phrase.Print(stream);
    return stream;
}
