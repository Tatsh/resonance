#include "game/joypad.h"

#include <libpad.h>

namespace {

// scePadGetState() values IsConnected() treats as absent.
constexpr int kPadStateDisconnected = 0;
constexpr int kPadStateClosed = 99;

} // namespace

// 0x004ec7d0
std::vector<bool> Joypad::sSlotsInUse(kSlotCount, false);

PadRecord Joypad::sRecords[kSlotCount];

// 0x004ec2b8
Joypad::Joypad(int nIndex) : mIndex(nIndex) {
    (void)sSlotsInUse[mIndex]; // Yes, the binary computes the bit reference once and discards it.
    sSlotsInUse[mIndex] = true;
}

// 0x004ec458
Joypad::~Joypad() {
    sSlotsInUse[mIndex] = false;
}

// 0x004ecaf8
int Joypad::Read(unsigned int *pButtons,
                 unsigned char *pAxis0,
                 unsigned char *pAxis1,
                 unsigned char *pAxis2,
                 unsigned char *pAxis3) {
    return sRecords[mIndex].Read(pButtons, pAxis0, pAxis1, pAxis2, pAxis3, nullptr, nullptr);
}

// 0x004ecb30
void Joypad::Reset() {
    sRecords[mIndex].mPhase = 0;
    sRecords[mIndex].mUnknown124 = 0;
}

// 0x004ecb60
void Joypad::Open(int nPort, int nSlot, int nUnknown128) {
    sRecords[mIndex].Open(nPort, nSlot, nUnknown128);
}

// 0x004ecb90
void Joypad::Close() {
    const PadRecord &record = sRecords[mIndex];
    scePadPortClose(record.mPort, record.mSlot);
}

// 0x004ecbc8
void Joypad::SetVibration(int nSmallMotor, int nBigMotor) {
    sRecords[mIndex].SetVibration(nSmallMotor, nBigMotor);
}

// 0x004ecbf8
int Joypad::IsConnected() {
    const PadRecord &record = sRecords[mIndex];
    const int nState = scePadGetState(record.mPort, record.mSlot);
    return nState != kPadStateDisconnected && nState != kPadStateClosed;
}
