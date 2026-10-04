#include "met/gameoptions.h"

GameOptions::GameOptions() : mStereo(1), mExpansionPack(0), mForceFeedback(1) {
}

GameOptions::~GameOptions() {
}

void GameOptions::Save(OBStream &stream) {
    stream << mExpansionPack << mForceFeedback << mStereo;
}

void GameOptions::Load(IBStream &stream) {
    stream >> mExpansionPack >> mForceFeedback >> mStereo;
}
