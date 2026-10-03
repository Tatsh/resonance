#include "game/gameparams.h"

#include <iostream>

namespace {

// NTSC-U/C: 0x0067e798, PAL: 0x006bf998
int g_nDoWinSequence;

} // namespace

// NTSC-U/C: 0x00187170, PAL: 0x0018c960
GameParams::GameParams() {
    mFriends = 0;
    mPlayMode = 0;
    mDifficulty = 0;
    mConstrainJam = 0;
    mNetGame = 0;
    mLoadingGame = 0;
    mJukeboxMode = 0;
}

// NTSC-U/C: 0x00187940, PAL: 0x0018d138
GameParams::~GameParams() {
}

// NTSC-U/C: 0x001871b8, PAL: 0x0018c9b0
void GameParams::Save(OBStream *pStream) const {
    unsigned nLevelNameLength = mLevelName.mLen;
    pStream->WriteLE(&nLevelNameLength, sizeof(nLevelNameLength));
    // An empty string has no buffer, and the stream receives the shared empty string in place of a
    // null pointer.
    pStream->Write(mLevelName.mStr != nullptr ? mLevelName.mStr : g_szEmptyString,
                   nLevelNameLength);

    unsigned nArenaNameLength = mArenaName.mLen;
    pStream->WriteLE(&nArenaNameLength, sizeof(nArenaNameLength));
    pStream->Write(mArenaName.mStr != nullptr ? mArenaName.mStr : g_szEmptyString,
                   nArenaNameLength);

    int nFriends = mFriends;
    pStream->WriteLE(&nFriends, sizeof(nFriends));

    int nPlayMode = mPlayMode;
    pStream->WriteLE(&nPlayMode, sizeof(nPlayMode));

    int difficulty = mDifficulty;
    pStream->WriteLE(&difficulty, sizeof(difficulty));

    int nConstrainJam = mConstrainJam;
    pStream->WriteLE(&nConstrainJam, sizeof(nConstrainJam));

    int nNetGame = mNetGame;
    pStream->WriteLE(&nNetGame, sizeof(nNetGame));

    int nLoadingGame = mLoadingGame;
    pStream->WriteLE(&nLoadingGame, sizeof(nLoadingGame));

    int nJukeboxMode = mJukeboxMode;
    pStream->WriteLE(&nJukeboxMode, sizeof(nJukeboxMode));
}

// NTSC-U/C: 0x00187390, PAL: 0x0018cb88
void GameParams::Load(IBStream *pStream) {
    unsigned nLevelNameLength;
    pStream->ReadLE(&nLevelNameLength, sizeof(nLevelNameLength));
    mLevelName.Alloc(nLevelNameLength);
    pStream->Read(mLevelName.mStr != nullptr ? mLevelName.mStr :
                                               const_cast<char *>(g_szEmptyString),
                  nLevelNameLength);

    unsigned nArenaNameLength;
    pStream->ReadLE(&nArenaNameLength, sizeof(nArenaNameLength));
    mArenaName.Alloc(nArenaNameLength);
    pStream->Read(mArenaName.mStr != nullptr ? mArenaName.mStr :
                                               const_cast<char *>(g_szEmptyString),
                  nArenaNameLength);

    pStream->ReadLE(&mFriends, sizeof(mFriends));

    // mPlayMode arrives in a local and is copied across afterwards, where mFriends and
    // mDifficulty are filled in place. Both are plain words, and the asymmetry matches the binary.
    int nPlayMode;
    pStream->ReadLE(&nPlayMode, sizeof(nPlayMode));

    pStream->ReadLE(&mDifficulty, sizeof(mDifficulty));

    int nConstrainJam;
    pStream->ReadLE(&nConstrainJam, sizeof(nConstrainJam));

    int nNetGame;
    pStream->ReadLE(&nNetGame, sizeof(nNetGame));

    int nLoadingGame;
    pStream->ReadLE(&nLoadingGame, sizeof(nLoadingGame));

    int nJukeboxMode;
    pStream->ReadLE(&nJukeboxMode, sizeof(nJukeboxMode));

    mPlayMode = nPlayMode;
    mConstrainJam = nConstrainJam != 0;
    mNetGame = nNetGame != 0;
    mLoadingGame = nLoadingGame != 0;
    mJukeboxMode = nJukeboxMode != 0;
}

// NTSC-U/C: 0x00187570, PAL: 0x0018cd68
void GameParams::Print(std::ostream &stream) const {
    stream << "GameParams:" << " level=" << mLevelName << " arena=" << mArenaName
           << " friends=" << mFriends << " " << (mPlayMode == 1 ? " game" : " jam")
           << " difficulty=" << mDifficulty << " " << (mConstrainJam ? " constrain-jam" : "")
           << "netgame=" << mNetGame << "loadinggame=" << mLoadingGame
           << "jukeboxmode=" << mJukeboxMode << std::endl;
}

// NTSC-U/C: 0x00187be8, PAL: 0x0018d470
GameParams &GameParams::operator=(const GameParams &other) {
    mLevelName = other.mLevelName;
    mArenaName = other.mArenaName;
    mFriends = other.mFriends;
    mPlayMode = other.mPlayMode;
    mDifficulty = other.mDifficulty;
    mConstrainJam = other.mConstrainJam;
    mNetGame = other.mNetGame;
    mLoadingGame = other.mLoadingGame;
    mJukeboxMode = other.mJukeboxMode;
    return *this;
}

// NTSC-U/C: 0x00187b20, PAL: 0x0018d3a8
bool GameParams::operator==(const GameParams &other) const {
    return mLevelName == other.mLevelName && mArenaName == other.mArenaName &&
           mFriends == other.mFriends && mPlayMode == other.mPlayMode &&
           mDifficulty == other.mDifficulty && mConstrainJam == other.mConstrainJam &&
           mNetGame == other.mNetGame && mLoadingGame == other.mLoadingGame &&
           mJukeboxMode == other.mJukeboxMode;
}

// NTSC-U/C: 0x00187b00, PAL: 0x0018d388
int GetDoWinSequence() {
    return g_nDoWinSequence;
}

// NTSC-U/C: 0x00187b10, PAL: 0x0018d398
void SetDoWinSequence(int nDoWinSequence) {
    g_nDoWinSequence = nDoWinSequence;
}
