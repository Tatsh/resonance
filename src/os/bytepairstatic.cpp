#include "os/bytepairstatic.h"

namespace {

// The value the constructor stores into every byte.
constexpr unsigned char kUnsetByte = 0xff;

} // namespace

// 0x00558d90
BytePairStatic::BytePairStatic() {
    for (int i = 1; i >= 0; --i) {
        mBytes[i] = kUnsetByte;
    }
}

// 0x00558dd0
BytePairStatic::~BytePairStatic() {
}

// 0x00558d10
BytePairStatic *BytePairStatic::shared() {
    static BytePairStatic instance;
    return &instance;
}
