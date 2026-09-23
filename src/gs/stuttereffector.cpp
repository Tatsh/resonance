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

// 0x001a0ae0
void StutterEffector::Enable(int bEnabled) {
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

// 0x001a0cc8
int StutterEffector::Tick(int nElapsedTicks) {
    float flValue;
    mOscillator->Sample(static_cast<float>(nElapsedTicks), &flValue);
    const double dLevel = (flValue * kLevelScale) + ((1.0 - flValue) * mFloor);
    StdMidiMsg msg(kMBTInfinity,
                   kStatusControlChange | mChannel,
                   kControllerLevel,
                   static_cast<unsigned char>(static_cast<unsigned int>(dLevel)));
    Send(&msg);
    return 1;
}

// 0x001a2338
StutterEffector::~StutterEffector() {
    StutterEffector::Enable(0); // The binary calls this class's own body rather than dispatching.
    delete mOscillator;
}

// 0x001a2420
int StutterEffector::Type() {
    return kEffectorTypeStutter;
}
