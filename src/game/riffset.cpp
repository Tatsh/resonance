#include "game/riffset.h"

#include <cstddef>
#include <cstring>
#include <iostream>

#include "game/riff.h"

// 0x001cecc0
RiffSet::RiffSet() {
    memset(mRiffs, 0, sizeof(mRiffs));
}

// 0x001cecf0
RiffSet::~RiffSet() {
    for (int i = 0; i < kRiffSetLevelCount; ++i) {
        if (mRiffs[i] != nullptr) {
            mRiffs[i]->Release();
        }
    }
}

// 0x001ced70
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
