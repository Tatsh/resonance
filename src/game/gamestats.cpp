#include "game/gamestats.h"

// 0x0010f150
GameStats::GameStats() {
    mPlayerCount = 0;
    mCompleted = 0;
    mUnknown10 = 0;
}

// 0x0010b648
GameStats::~GameStats() {
}

// 0x0010f1a8
void GameStats::Reset(int nPlayers) {
    mCompleted = 0;
    mUnknown08 = 0;
    mPlayerCount = nPlayers;
    mUnknown10 = 0;
    mProgress = 0.0f;
    mUnknown14 = 0;

    mScores.reserve(nPlayers);
    mRatios.reserve(nPlayers);
    mTallies.reserve(nPlayers);
    for (int i = 0; i < nPlayers; ++i) {
        mScores.push_back(0);
        mRatios.push_back(0.0f);
        mTallies.push_back(0);
    }
}

// 0x0010ff20
int GameStats::GetScore(int nPlayer) {
    return mScores[nPlayer];
}

// 0x0010ff38
void GameStats::SetScore(int nPlayer, int nScore) {
    mScores[nPlayer] = nScore;
}

// 0x0010ff50
float GameStats::GetProgress() {
    return mProgress;
}

// 0x0010ff58
void GameStats::SetProgress(float flProgress) {
    mProgress = flProgress;
}

// 0x0010ff60
float GameStats::GetRatio(int nPlayer) {
    return mRatios[nPlayer];
}

// 0x0010ff78
void GameStats::SetRatio(int nPlayer, float flRatio) {
    mRatios[nPlayer] = flRatio;
}

// 0x0010ff90
int GameStats::GetTally(int nPlayer) {
    return mTallies[nPlayer];
}

// 0x0010ffa8
void GameStats::SetTally(int nPlayer, int nTally) {
    mTallies[nPlayer] = nTally;
}
