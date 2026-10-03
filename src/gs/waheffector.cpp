#include "gs/waheffector.h"

#include "msg/stdmidimsg.h"

namespace {

constexpr unsigned char kStatusControlChange = 0xb0;
constexpr unsigned char kControllerSweep = 0x4a;
constexpr unsigned char kControllerSwitch = 0x51;
constexpr unsigned char kSwitchOn = 0x7f;
constexpr unsigned char kSwitchOff = 0;

} // namespace

// NTSC-U/C: 0x001a08f8, PAL: 0x001a6660
void WahEffector::SetEnabled(int bEnabled) {
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

// NTSC-U/C: 0x001a0a10, PAL: 0x001a6778
int WahEffector::Tick(int nElapsedTicks) {
    float flValue;
    mOscillator->GetValue(static_cast<float>(nElapsedTicks), &flValue);
    const int nLevel = static_cast<int>(static_cast<float>(mDepth) * flValue);
    StdMidiMsg msg(kMBTInfinity,
                   kStatusControlChange | mChannel,
                   kControllerSweep,
                   static_cast<unsigned char>(nLevel));
    Send(&msg);
    return 1;
}

// NTSC-U/C: 0x001a2040, PAL: 0x001a7da8
WahEffector::~WahEffector() {
    WahEffector::SetEnabled(0); // The binary calls this class's own body rather than dispatching.
    delete mOscillator;
}

// NTSC-U/C: 0x001a2128, PAL: 0x001a7e90
int WahEffector::Type() {
    return kEffectorTypeWah;
}
