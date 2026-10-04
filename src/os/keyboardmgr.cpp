#include "os/keyboardmgr.h"

namespace {

// The value InitPortMap() stores into every byte.
constexpr unsigned char kUnsetByte = 0xff;

} // namespace

KeyboardMgr::KeyboardMgr() {
    InitPortMap();
}

void KeyboardMgr::InitPortMap() {
    for (int i = 1; i >= 0; --i) {
        mBytes[i] = kUnsetByte;
    }
}

KeyboardMgr::~KeyboardMgr() {
}

KeyboardMgr *KeyboardMgr::shared() {
    static KeyboardMgr instance;
    return &instance;
}
