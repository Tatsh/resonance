#include "gs/volumeeffector.h"

#include "mid/mbt.h"
#include "msg/stdmidimsg.h"

namespace {

// A control-change status, before the channel is combined into its low nibble.
constexpr unsigned char kStatusControlChange = 0xb0;

// The controller the effect drives, and the value it restores when switched off.
constexpr unsigned char kVolumeController = 0x2f;
constexpr unsigned char kControllerFull = 0x7f;

} // namespace

// 0x001a1a10
VolumeEffector::VolumeEffector(unsigned char nChannel, int nValue)
    : mChannel(nChannel), mValue(nValue), mEnabled(0) {
}

// 0x001a1a68
VolumeEffector::~VolumeEffector() {
    Enable(0);
}

// 0x001a1b30
int VolumeEffector::Type() {
    return kEffectorTypeVolume;
}

// 0x001a07c0
void VolumeEffector::Enable(int bEnabled) {
    if (bEnabled == mEnabled) {
        return;
    }
    mEnabled = bEnabled;
    const unsigned char nValue = bEnabled ? static_cast<unsigned char>(mValue) : kControllerFull;
    StdMidiMsg msg(kMBTInfinity, kStatusControlChange | mChannel, kVolumeController, nValue);
    Send(&msg);
}
