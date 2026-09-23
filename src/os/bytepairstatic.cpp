#include "os/bytepairstatic.h"

namespace {

// The value Reset() stores into every byte.
constexpr unsigned char kUnsetByte = 0xff;

} // namespace

// 0x00558d68
BytePairStatic::BytePairStatic() {
    Reset();
}

// 0x00558d90
void BytePairStatic::Reset() {
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
