#include "met/gameoptions.h"

// 0x0032e780
GameOptions::GameOptions() : mStereo(1), mExpansionPack(0), mForceFeedback(1) {
}

// 0x0032e798
GameOptions::~GameOptions() {
}

// 0x0032e7c8
void GameOptions::Save(OBStream &stream) {
    stream << mExpansionPack << mForceFeedback << mStereo;
}

// 0x0032e810
void GameOptions::Load(IBStream &stream) {
    stream >> mExpansionPack >> mForceFeedback >> mStereo;
}
