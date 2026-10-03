#include "game/riffset.h"

#include <cstddef>
#include <cstring>
#include <iostream>

#include "game/riff.h"

// NTSC-U/C: 0x001cecc0, PAL: 0x001d4b78
RiffSet::RiffSet() {
    memset(mRiffs, 0, sizeof(mRiffs));
}

// NTSC-U/C: 0x001cecf0, PAL: 0x001d4ba8
RiffSet::~RiffSet() {
    for (int i = 0; i < kRiffSetLevelCount; ++i) {
        if (mRiffs[i] != nullptr) {
            mRiffs[i]->Release();
        }
    }
}

// NTSC-U/C: 0x001ced70, PAL: 0x001d4c28
void RiffSet::Print(std::ostream &stream) {
    for (int i = 0; i < kRiffSetLevelCount; ++i) {
        stream << i << ":";
        if (mRiffs[i] != nullptr) {
            mRiffs[i]->Print(stream);
            stream << std::endl;
        } else {
            stream << "[empty]" << std::endl;
        }
    }
}
