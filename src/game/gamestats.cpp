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

int GameStats::GetScore(int nPlayer) {
    return mScores[nPlayer];
}

void GameStats::SetScore(int nPlayer, int nScore) {
    mScores[nPlayer] = nScore;
}

float GameStats::GetProgress() {
    return mProgress;
}

void GameStats::SetProgress(float flProgress) {
    mProgress = flProgress;
}

float GameStats::GetRatio(int nPlayer) {
    return mRatios[nPlayer];
}

void GameStats::SetRatio(int nPlayer, float flRatio) {
    mRatios[nPlayer] = flRatio;
}

int GameStats::GetTally(int nPlayer) {
    return mTallies[nPlayer];
}

void GameStats::SetTally(int nPlayer, int nTally) {
    mTallies[nPlayer] = nTally;
}
