#include "game/inputpoller.h"
#include "sch/command.h"

namespace {

/**
 * Scheduler command the controller scan runs.
 *
 * `Q235_GLOBAL_$N$InputPollerPS2.cppXFKhgb24FindJoypadConnectionsCmd` at `0x007e6ae8` is its RTTI
 * name. Only the factory is recovered; no vtable that references the class has been located.
 */
class FindJoypadConnectionsCmd : public Sch::Command {
public:
    // 0x001e1990
    // The factory the unit's static initialiser at 0x001e1750 registers under identifier zero.
    static Sch::Command *NewCmd() {
        return nullptr;
    }
};

} // namespace

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
