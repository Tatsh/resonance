#include "game/inputpoller.h"

// 0x001e1998
void InputPoller::SetController(RawController *pController) {
    mController = pController;
}

// 0x001e1a80
void InputPoller::SetActive(int bActive) {
    mActive = bActive;
}

// 0x001e19a0
void InputPoller::DetachController(RawController *pController) {
    if (mController == pController) {
        mController = nullptr;
    }
}

// 0x001e1c18
void InputPoller::SetPaused(int bPaused) {
    mPaused = bPaused;
}
