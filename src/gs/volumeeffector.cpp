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

// NTSC-U/C: 0x001a1a10, PAL: 0x001a7778
VolumeEffector::VolumeEffector(unsigned char nChannel, int nAppliedLevel)
    : mChannel(nChannel), mAppliedLevel(nAppliedLevel), mEnabled(0) {
}

// NTSC-U/C: 0x001a1a68, PAL: 0x001a77d0
VolumeEffector::~VolumeEffector() {
    SetEnabled(0);
}

// NTSC-U/C: 0x001a1b30, PAL: 0x001a7898
int VolumeEffector::Type() {
    return kEffectorTypeVolume;
}

// NTSC-U/C: 0x001a07c0, PAL: 0x001a6528
void VolumeEffector::SetEnabled(int bEnabled) {
    if (bEnabled == mEnabled) {
        return;
    }
    mEnabled = bEnabled;
    const unsigned char nValue =
        bEnabled ? static_cast<unsigned char>(mAppliedLevel) : kControllerFull;
    StdMidiMsg msg(kMBTInfinity, kStatusControlChange | mChannel, kVolumeController, nValue);
    Send(&msg);
}
