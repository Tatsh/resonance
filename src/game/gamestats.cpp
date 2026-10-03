#include "game/gamestats.h"

// NTSC-U/C: 0x0010f150, PAL: 0x0010f5b0
GameStats::GameStats() {
    mPlayerCount = 0;
    mCompleted = 0;
    mUnreadCounter = 0;
}

// NTSC-U/C: 0x0010b648, PAL: 0x0010b7d0
GameStats::~GameStats() {
}

// NTSC-U/C: 0x0010f1a8, PAL: 0x0010f608
void GameStats::Reset(int nPlayers) {
    mCompleted = 0;
    mCheated = 0;
    mPlayerCount = nPlayers;
    mUnreadCounter = 0;
    mProgress = 0.0f;
    mRemixEdited = 0;

    mScores.reserve(nPlayers);
    mRatios.reserve(nPlayers);
    mTallies.reserve(nPlayers);
    for (int i = 0; i < nPlayers; ++i) {
        mScores.push_back(0);
        mRatios.push_back(0.0f);
        mTallies.push_back(0);
    }
}

// NTSC-U/C: 0x0010ff20, PAL: 0x00110380
int GameStats::GetScore(int nPlayer) {
    return mScores[nPlayer];
}

// NTSC-U/C: 0x0010ff38, PAL: 0x00110398
void GameStats::SetScore(int nPlayer, int nScore) {
    mScores[nPlayer] = nScore;
}

// NTSC-U/C: 0x0010ff50, PAL: 0x001103b0
float GameStats::GetProgress() {
    return mProgress;
}

// NTSC-U/C: 0x0010ff58, PAL: 0x001103b8
void GameStats::SetProgress(float flProgress) {
    mProgress = flProgress;
}

// NTSC-U/C: 0x0010ff60, PAL: 0x001103c0
float GameStats::GetRatio(int nPlayer) {
    return mRatios[nPlayer];
}

// NTSC-U/C: 0x0010ff78, PAL: 0x001103d8
void GameStats::SetRatio(int nPlayer, float flRatio) {
    mRatios[nPlayer] = flRatio;
}

// NTSC-U/C: 0x0010ff90, PAL: 0x001103f0
int GameStats::GetTally(int nPlayer) {
    return mTallies[nPlayer];
}

// NTSC-U/C: 0x0010ffa8, PAL: 0x00110408
void GameStats::SetTally(int nPlayer, int nTally) {
    mTallies[nPlayer] = nTally;
}
