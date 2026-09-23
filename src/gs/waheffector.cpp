#include "gs/waheffector.h"

#include "msg/stdmidimsg.h"

namespace {

constexpr unsigned char kStatusControlChange = 0xb0;
constexpr unsigned char kControllerSweep = 0x4a;
constexpr unsigned char kControllerSwitch = 0x51;
constexpr unsigned char kSwitchOn = 0x7f;
constexpr unsigned char kSwitchOff = 0;

} // namespace

// 0x001a08f8
void WahEffector::Enable(int bEnabled) {
    if (bEnabled == mEnabled && mPending != 0) {
        return;
    }
    mEnabled = bEnabled;
    mPending = 1;

    StdMidiMsg msg(kMBTInfinity,
                   kStatusControlChange | mChannel,
                   kControllerSwitch,
                   bEnabled != 0 ? kSwitchOn : kSwitchOff);
    Send(&msg);
}

// 0x001a0a10
int WahEffector::Tick(int nElapsedTicks) {
    float flValue;
    mOscillator->Sample(static_cast<float>(nElapsedTicks), &flValue);
    const int nLevel = static_cast<int>(static_cast<float>(mDepth) * flValue);
    StdMidiMsg msg(kMBTInfinity,
                   kStatusControlChange | mChannel,
                   kControllerSweep,
                   static_cast<unsigned char>(nLevel));
    Send(&msg);
    return 1;
}

// 0x001a2040
WahEffector::~WahEffector() {
    WahEffector::Enable(0); // The binary calls this class's own body rather than dispatching.
    delete mOscillator;
}

// 0x001a2128
int WahEffector::Type() {
    return kEffectorTypeWah;
}
