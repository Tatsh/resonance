#include "gs/midionoffeffector.h"

#include "mid/tick.h"
#include "msg/stdmidimsg.h"

namespace {

// A control-change status, before the channel is combined into its low nibble.
constexpr unsigned char kStatusControlChange = 0xb0;

// The two controller values the effect sends.
constexpr unsigned char kControllerOn = 0x7f;
constexpr unsigned char kControllerOff = 0;

} // namespace

MidiOnOffEffector::MidiOnOffEffector(unsigned char nChannel, int nType, unsigned char nController)
    : mType(nType), mChannel(nChannel), mEnabled(0), mController(nController) {
}

MidiOnOffEffector::~MidiOnOffEffector() {
    SetEnabled(0);
}

int MidiOnOffEffector::Type() {
    return mType;
}

void MidiOnOffEffector::SetEnabled(int bEnabled) {
    if (bEnabled == mEnabled) {
        return;
    }
    StdMidiMsg msg(kTickInfinity,
                   kStatusControlChange | mChannel,
                   mController,
                   bEnabled ? kControllerOn : kControllerOff);
    mEnabled = bEnabled;
    Send(&msg);
}
