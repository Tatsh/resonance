#include "game/padrecord.h"

#include <libpad.h>

namespace {

// Byte of the unused alignment entries.
constexpr unsigned char kActuatorUnused = 0xff;

// Report bytes the first two actuators take, which leaves the other four unused.
constexpr unsigned char kSmallMotorByte = 0;
constexpr unsigned char kBigMotorByte = 1;

// Actuator entries of the small and big motors.
enum ActuatorIndex { kSmallMotor = 0, kBigMotor = 1 };

// mUnknown124 from which SetVibration() drives the motors.
constexpr int kVibrationReady = 2;

} // namespace

int PadRecord::sPadLibraryStarted;

// 0x005bcf58
void PadRecord::Open(int nPort, int nSlot, int nUnknown128) {
    for (int i = 0; i < kActuatorByteCount; ++i) {
        mActDirect[i] = 0;
        mActAlign[i] = kActuatorUnused;
    }
    for (int i = kPressureByteCount - 1; i >= 0; --i) {
        mPressureBaseline[i] = 0;
    }
    mActAlign[kSmallMotor] = kSmallMotorByte;
    mActAlign[kBigMotor] = kBigMotorByte;
    mPort = nPort;
    mSlot = nSlot;
    mPhase = 0;
    mUnknown118 = 0;
    mUnknown12c = 0;
    mUnknown120 = 0;
    if (sPadLibraryStarted == 0) {
        scePadInit(0);
        sPadLibraryStarted = 1;
    }
    for (auto &byte : mUnknown130) {
        byte = 0;
    }
    for (auto &value : mUnknown134) {
        value = 0;
    }
    scePadPortOpen(nPort, nSlot, mDmaArea);
    mUnknown128 = nUnknown128;
    mButtons = 0;
    mUnknown104 = 0;
    mUnknown108 = 0;
    mUnknown10c = 0;
}

// 0x005bd0b0
void PadRecord::SetVibration(int nSmallMotor, int nBigMotor) {
    if (mUnknown124 < kVibrationReady) {
        return;
    }
    mActDirect[kSmallMotor] = nSmallMotor > 0;
    mActDirect[kBigMotor] = nBigMotor;
    scePadSetActDirect(mPort, mSlot, mActDirect);
}
