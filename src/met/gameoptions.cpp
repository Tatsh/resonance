#include "met/gameoptions.h"

// NTSC-U/C: 0x0032e780, PAL: 0x00356cc8
GameOptions::GameOptions() : mStereo(1), mExpansionPack(0), mForceFeedback(1) {
}

// NTSC-U/C: 0x0032e798, PAL: 0x00356ce0
GameOptions::~GameOptions() {
}

// NTSC-U/C: 0x0032e7c8, PAL: 0x00356d10
void GameOptions::Save(OBStream &stream) {
    stream << mExpansionPack << mForceFeedback << mStereo;
}

// NTSC-U/C: 0x0032e810, PAL: 0x00356d58
void GameOptions::Load(IBStream &stream) {
    stream >> mExpansionPack >> mForceFeedback >> mStereo;
}
