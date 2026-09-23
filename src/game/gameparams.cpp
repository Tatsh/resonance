#include "game/gameparams.h"

#include <iostream>

namespace {

// 0x0067e798
int g_nDoWinSequence;

} // namespace

// 0x00187170
GameParams::GameParams() {
    mUnknown10 = 0;
    mUnknown1c = 0;
    mDifficulty = 0;
    mUnknown24 = false;
    mUnknown28 = false;
    mLoadingGame = false;
    mJukeboxMode = false;
}

// 0x00187940
GameParams::~GameParams() {
}

// 0x001871b8
void GameParams::Save(OBStream *pStream) {
    unsigned length00 = mLevelName.mLen;
    pStream->Write(&length00, sizeof(length00));
    // An empty string has no buffer, and the stream receives the shared empty string in place of a
    // null pointer.
    pStream->WriteBytes(mLevelName.mStr != nullptr ? mLevelName.mStr : g_szEmptyString, length00);

    unsigned length08 = mArenaName.mLen;
    pStream->Write(&length08, sizeof(length08));
    pStream->WriteBytes(mArenaName.mStr != nullptr ? mArenaName.mStr : g_szEmptyString, length08);

    int unknown10 = mUnknown10;
    pStream->Write(&unknown10, sizeof(unknown10));

    int unknown1c = mUnknown1c;
    pStream->Write(&unknown1c, sizeof(unknown1c));

    int difficulty = mDifficulty;
    pStream->Write(&difficulty, sizeof(difficulty));

    int unknown24 = mUnknown24;
    pStream->Write(&unknown24, sizeof(unknown24));

    int unknown28 = mUnknown28;
    pStream->Write(&unknown28, sizeof(unknown28));

    int unknown2c = mLoadingGame;
    pStream->Write(&unknown2c, sizeof(unknown2c));

    int unknown30 = mJukeboxMode;
    pStream->Write(&unknown30, sizeof(unknown30));
}

// 0x00187390
void GameParams::Load(IBStream *pStream) {
    unsigned length00;
    pStream->Read(&length00, sizeof(length00));
    mLevelName.Alloc(length00);
    pStream->ReadBytes(mLevelName.mStr != nullptr ? mLevelName.mStr :
                                                    const_cast<char *>(g_szEmptyString),
                       length00);

    unsigned length08;
    pStream->Read(&length08, sizeof(length08));
    mArenaName.Alloc(length08);
    pStream->ReadBytes(mArenaName.mStr != nullptr ? mArenaName.mStr :
                                                    const_cast<char *>(g_szEmptyString),
                       length08);

    pStream->Read(&mUnknown10, sizeof(mUnknown10));

    // mUnknown1c arrives in a local and is copied across afterwards, where mUnknown10 and
    // mDifficulty are filled in place. Both are plain words, and the asymmetry matches the binary.
    int unknown1c;
    pStream->Read(&unknown1c, sizeof(unknown1c));

    pStream->Read(&mDifficulty, sizeof(mDifficulty));

    int unknown24;
    pStream->Read(&unknown24, sizeof(unknown24));

    int unknown28;
    pStream->Read(&unknown28, sizeof(unknown28));

    int unknown2c;
    pStream->Read(&unknown2c, sizeof(unknown2c));

    int unknown30;
    pStream->Read(&unknown30, sizeof(unknown30));

    mUnknown1c = unknown1c;
    mUnknown24 = unknown24 != 0;
    mUnknown28 = unknown28 != 0;
    mLoadingGame = unknown2c != 0;
    mJukeboxMode = unknown30 != 0;
}

// 0x00187570
void GameParams::Print(std::ostream &stream) {
    stream << "GameParams:" << " level=" << mLevelName << " arena=" << mArenaName
           << " friends=" << mUnknown10 << " " << (mUnknown1c == 1 ? " game" : " jam")
           << " difficulty=" << mDifficulty << " " << (mUnknown24 ? " constrain-jam" : "")
           << "netgame=" << mUnknown28 << "loadinggame=" << mLoadingGame
           << "jukeboxmode=" << mJukeboxMode << std::endl;
}

// 0x00187be8
GameParams &GameParams::operator=(const GameParams &other) {
    mLevelName = other.mLevelName;
    mArenaName = other.mArenaName;
    mUnknown10 = other.mUnknown10;
    mUnknown1c = other.mUnknown1c;
    mDifficulty = other.mDifficulty;
    mUnknown24 = other.mUnknown24;
    mUnknown28 = other.mUnknown28;
    mLoadingGame = other.mLoadingGame;
    mJukeboxMode = other.mJukeboxMode;
    return *this;
}

// 0x00187b00
int GetDoWinSequence() {
    return g_nDoWinSequence;
}

// 0x00187b10
void SetDoWinSequence(int nDoWinSequence) {
    g_nDoWinSequence = nDoWinSequence;
}
