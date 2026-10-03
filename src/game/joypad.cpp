#include "game/joypad.h"

#include <libpad.h>

namespace {

// scePadGetState() values IsConnected() treats as absent.
constexpr int kPadStateDisconnected = 0;
constexpr int kPadStateClosed = 99;

} // namespace

// NTSC-U/C: 0x004ec7d0, PAL: 0x0052b358
std::vector<bool> Joypad::sSlotsInUse(kSlotCount, false);

PadRecord Joypad::sRecords[kSlotCount];

// NTSC-U/C: 0x004ec2b8, PAL: 0x0052ae40
Joypad::Joypad(int nIndex) : mIndex(nIndex) {
    (void)sSlotsInUse[mIndex]; // Yes, the binary computes the bit reference once and discards it.
    sSlotsInUse[mIndex] = true;
}

// NTSC-U/C: 0x004ec458, PAL: 0x0052afe0
Joypad::~Joypad() {
    sSlotsInUse[mIndex] = false;
}

// NTSC-U/C: 0x004ecaf8, PAL: 0x0052b680
int Joypad::Read(unsigned int *pButtons,
                 unsigned char *pAxis0,
                 unsigned char *pAxis1,
                 unsigned char *pAxis2,
                 unsigned char *pAxis3) {
    return sRecords[mIndex].Read(pButtons, pAxis0, pAxis1, pAxis2, pAxis3, nullptr, nullptr);
}

// NTSC-U/C: 0x004ecb30, PAL: 0x0052b6d8
void Joypad::Reset() {
    sRecords[mIndex].mPhase = 0;
    sRecords[mIndex].mReadyLevel = 0;
}

// NTSC-U/C: 0x004ecb60, PAL: 0x0052b708
void Joypad::Open(int nPort, int nSlot, int nDeadZone) {
    sRecords[mIndex].Open(nPort, nSlot, nDeadZone);
}

// NTSC-U/C: 0x004ecb90, PAL: 0x0052b738
void Joypad::Close() {
    const PadRecord &record = sRecords[mIndex];
    scePadPortClose(record.mPort, record.mSlot);
}

// NTSC-U/C: 0x004ecbc8, PAL: 0x0052b770
void Joypad::SetVibration(int nSmallMotor, int nBigMotor) {
    sRecords[mIndex].SetVibration(nSmallMotor, nBigMotor);
}

// NTSC-U/C: 0x004ecbf8, PAL: 0x0052b7a0
int Joypad::IsConnected() {
    const PadRecord &record = sRecords[mIndex];
    const int nState = scePadGetState(record.mPort, record.mSlot);
    return nState != kPadStateDisconnected && nState != kPadStateClosed;
}

#ifdef VIDEO_STANDARD_PAL
// PAL: 0x0052b6b8
void Joypad::EndLibrary() {
    PadRecord::EndLibrary();
}
#endif
