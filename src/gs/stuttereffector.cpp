#include "gs/stuttereffector.h"

#include "app/application.h"
#include "msg/stdmidimsg.h"
#include "sch/tickclock.h"

namespace {

constexpr unsigned char kStatusControlChange = 0xb0;
constexpr unsigned char kControllerLevel = 0x30;
constexpr unsigned char kControllerSwitch = 0x50;
constexpr unsigned char kSwitchOn = 0x7f;
constexpr unsigned char kSwitchOff = 0;
constexpr unsigned char kFullLevel = 0x7f;

// The oscillator's value blends the full level against mFloor.
constexpr float kLevelScale = 127.0f;

} // namespace

// NTSC-U/C: 0x001a0ae0, PAL: 0x001a6848
void StutterEffector::SetEnabled(int bEnabled) {
    if (bEnabled == mEnabled && mPending != 0) {
        return;
    }
    mEnabled = bEnabled;
    mPending = 1;

    if (bEnabled != 0) {
        Tick(Application::shared()->GetSongClock()->SongTick());
        StdMidiMsg on(kMBTInfinity, kStatusControlChange | mChannel, kControllerSwitch, kSwitchOn);
        Send(&on);
        Start(kMBTInfinity);
        return;
    }

    Stop();
    StdMidiMsg level(kMBTInfinity, kStatusControlChange | mChannel, kControllerLevel, kFullLevel);
    Send(&level);
    StdMidiMsg off(kMBTInfinity, kStatusControlChange | mChannel, kControllerSwitch, kSwitchOff);
    Send(&off);
}

// NTSC-U/C: 0x001a0cc8, PAL: 0x001a6a30
int StutterEffector::Tick(int nElapsedTicks) {
    float flValue;
    mOscillator->GetValue(static_cast<float>(nElapsedTicks), &flValue);
    const double dLevel = (flValue * kLevelScale) + ((1.0 - flValue) * mFloor);
    StdMidiMsg msg(kMBTInfinity,
                   kStatusControlChange | mChannel,
                   kControllerLevel,
                   static_cast<unsigned char>(static_cast<unsigned int>(dLevel)));
    Send(&msg);
    return 1;
}

// NTSC-U/C: 0x001a2338, PAL: 0x001a80a0
StutterEffector::~StutterEffector() {
    // The binary calls this class's own body rather than dispatching.
    StutterEffector::SetEnabled(0);
    delete mOscillator;
}

// NTSC-U/C: 0x001a2420, PAL: 0x001a8188
int StutterEffector::Type() {
    return kEffectorTypeStutter;
}
