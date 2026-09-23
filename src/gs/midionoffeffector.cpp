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

// 0x001a1bc0
MidiOnOffEffector::MidiOnOffEffector(unsigned char nChannel, int nType, unsigned char nController)
    : mType(nType), mChannel(nChannel), mEnabled(0), mController(nController) {
}

// 0x001a1c28
MidiOnOffEffector::~MidiOnOffEffector() {
    Enable(0);
}

// 0x001a1cf0
int MidiOnOffEffector::Type() {
    return mType;
}

// 0x001a0860
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
