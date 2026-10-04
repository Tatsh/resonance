#include "game/joypadps2.h"

#include <libpad.h>

namespace {

// scePadGetState() values IsConnected() treats as absent.
constexpr int kPadStateDisconnected = 0;
constexpr int kPadStateClosed = 99;

} // namespace

// NTSC-U/C: 0x004ec7d0, PAL: 0x0052b358
std::vector<bool> JoypadPS2::sSlotsInUse(kSlotCount, false);

PadRecord JoypadPS2::sRecords[kSlotCount];

JoypadPS2::JoypadPS2(int nIndex) : mIndex(nIndex) {
    (void)sSlotsInUse[mIndex]; // Yes, the binary computes the bit reference once and discards it.
    sSlotsInUse[mIndex] = true;
}

JoypadPS2::~JoypadPS2() {
    sSlotsInUse[mIndex] = false;
}

int JoypadPS2::Poll(unsigned int *pButtons,
                    unsigned char *pAxis0,
                    unsigned char *pAxis1,
                    unsigned char *pAxis2,
                    unsigned char *pAxis3) {
    return BreugPadRead(
        &sRecords[mIndex], pButtons, pAxis0, pAxis1, pAxis2, pAxis3, nullptr, nullptr);
}

void JoypadPS2::Reset() {
    sRecords[mIndex].mPhase = 0;
    sRecords[mIndex].mReadyLevel = 0;
}

void JoypadPS2::Open(int nPort, int nSlot, int nDeadZone) {
    BreugPadInit(&sRecords[mIndex], nPort, nSlot, nDeadZone);
}

void JoypadPS2::DeInitPadData() {
    const PadRecord &record = sRecords[mIndex];
    scePadPortClose(record.mPort, record.mSlot);
}

void JoypadPS2::SetVibration(int nSmallMotor, int nBigMotor) {
    BreugPadSetMotors(&sRecords[mIndex], nSmallMotor, nBigMotor);
}

int JoypadPS2::IsConnected() {
    const PadRecord &record = sRecords[mIndex];
    const int nState = scePadGetState(record.mPort, record.mSlot);
    return nState != kPadStateDisconnected && nState != kPadStateClosed;
}

#ifdef VIDEO_STANDARD_PAL
void JoypadPS2::EndLibrary() {
    PadRecord::EndLibrary();
}
#endif
