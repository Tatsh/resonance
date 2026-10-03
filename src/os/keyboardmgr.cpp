#include "os/keyboardmgr.h"

namespace {

// The value InitPortMap() stores into every byte.
constexpr unsigned char kUnsetByte = 0xff;

} // namespace

// NTSC-U/C: 0x00558d68, PAL: 0x00599ec0
KeyboardMgr::KeyboardMgr() {
    InitPortMap();
}

// NTSC-U/C: 0x00558d90, PAL: 0x00599ee8
void KeyboardMgr::InitPortMap() {
    for (int i = 1; i >= 0; --i) {
        mBytes[i] = kUnsetByte;
    }
}

// NTSC-U/C: 0x00558dd0, PAL: 0x00599f28
KeyboardMgr::~KeyboardMgr() {
}

// NTSC-U/C: 0x00558d10, PAL: 0x00599e68
KeyboardMgr *KeyboardMgr::shared() {
    static KeyboardMgr instance;
    return &instance;
}
