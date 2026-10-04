#include "gs/volumeeffector.h"

#include "mid/tick.h"
#include "msg/stdmidimsg.h"

namespace {

// A control-change status, before the channel is combined into its low nibble.
constexpr unsigned char kStatusControlChange = 0xb0;

// The controller the effect drives, and the value it restores when switched off.
constexpr unsigned char kVolumeController = 0x2f;
constexpr unsigned char kControllerFull = 0x7f;

} // namespace

VolumeEffector::VolumeEffector(unsigned char nChannel, int nAppliedLevel)
    : mChannel(nChannel), mAppliedLevel(nAppliedLevel), mEnabled(0) {
}

VolumeEffector::~VolumeEffector() {
    SetEnabled(0);
}

int VolumeEffector::Type() {
    return kEffectorTypeVolume;
}

void VolumeEffector::SetEnabled(int bEnabled) {
    if (bEnabled == mEnabled) {
        return;
    }
    mEnabled = bEnabled;
    const unsigned char nValue =
        bEnabled ? static_cast<unsigned char>(mAppliedLevel) : kControllerFull;
    StdMidiMsg msg(kTickInfinity, kStatusControlChange | mChannel, kVolumeController, nValue);
    Send(&msg);
}
