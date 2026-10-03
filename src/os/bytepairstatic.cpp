#include "os/bytepairstatic.h"

namespace {

// The value Reset() stores into every byte.
constexpr unsigned char kUnsetByte = 0xff;

} // namespace

// NTSC-U/C: 0x00558d68, PAL: 0x00599ec0
BytePairStatic::BytePairStatic() {
    Reset();
}

// NTSC-U/C: 0x00558d90, PAL: 0x00599ee8
void BytePairStatic::Reset() {
    for (int i = 1; i >= 0; --i) {
        mBytes[i] = kUnsetByte;
    }
}

// NTSC-U/C: 0x00558dd0, PAL: 0x00599f28
BytePairStatic::~BytePairStatic() {
}

// NTSC-U/C: 0x00558d10, PAL: 0x00599e68
BytePairStatic *BytePairStatic::shared() {
    static BytePairStatic instance;
    return &instance;
}
