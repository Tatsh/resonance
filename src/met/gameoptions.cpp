#include "met/gameoptions.h"

// 0x0032e780
GameOptions::GameOptions() : mUnknown00(1), mUnknown04(0), mUnknown08(1) {
}

// 0x0032e798
GameOptions::~GameOptions() {
}

// 0x0032e7c8
void GameOptions::Save(OBStream &stream) {
    stream << mUnknown04 << mUnknown08 << mUnknown00;
}

// 0x0032e810
void GameOptions::Load(IBStream &stream) {
    stream >> mUnknown04 >> mUnknown08 >> mUnknown00;
}
