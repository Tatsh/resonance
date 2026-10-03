#include "gs/midionoffeffector.h"

#include "mid/mbt.h"
#include "msg/stdmidimsg.h"

namespace {

// A control-change status, before the channel is combined into its low nibble.
constexpr unsigned char kStatusControlChange = 0xb0;

// The two controller values the effect sends.
constexpr unsigned char kControllerOn = 0x7f;
constexpr unsigned char kControllerOff = 0;

} // namespace

// NTSC-U/C: 0x001a1bc0, PAL: 0x001a7928
MidiOnOffEffector::MidiOnOffEffector(unsigned char nChannel, int nType, unsigned char nController)
    : mType(nType), mChannel(nChannel), mEnabled(0), mController(nController) {
}

// NTSC-U/C: 0x001a1c28, PAL: 0x001a7990
MidiOnOffEffector::~MidiOnOffEffector() {
    Enable(0);
}

// NTSC-U/C: 0x001a1cf0, PAL: 0x001a7a58
int MidiOnOffEffector::Type() {
    return mType;
}

// NTSC-U/C: 0x001a0860, PAL: 0x001a65c8
void MidiOnOffEffector::Enable(int bEnabled) {
    if (bEnabled == mEnabled) {
        return;
    }
    StdMidiMsg msg(kMBTInfinity,
                   kStatusControlChange | mChannel,
                   mController,
                   bEnabled ? kControllerOn : kControllerOff);
    mEnabled = bEnabled;
    Send(&msg);
}
