#include "game/padrecord.h"

#include <cstdlib>
#include <libpad.h>

#include "os/log.h"

namespace {

// Pad states scePadGetState() reports.
constexpr int kPadStateDisconnected = 0;
constexpr int kPadStateFindCtp1 = 2;
constexpr int kPadStateStable = 6;

// Terms scePadInfoMode() takes, and the identifiers Read() recognises.
constexpr int kInfoModeCurrentId = 1;
constexpr int kInfoModeCurrentExtendedId = 2;
constexpr int kPadIdAnalog = 4;
constexpr int kPadIdDualShock2 = 7;

// Progress scePadGetReqState() reports.
constexpr int kReqStateComplete = 0;
constexpr int kReqStateFailed = 1;
constexpr int kReqStateBusy = 2;

// The main mode Read() requests, analog and locked.
constexpr int kMainModeAnalog = 1;
constexpr int kMainModeLock = 3;

// The actuator scePadInfoAct() takes to report the actuator count.
constexpr int kInfoActCount = -1;

// A successful request of the two calls that report success as 1.
constexpr int kPadCallSucceeded = 1;

// Setup phases. Every other index of the image's 78-entry table does nothing.
enum Phase {
    kPhaseProbe = 0,
    kPhaseAnalogProbe = 40,
    kPhaseAnalogSetMode = 41,
    kPhaseAnalogWait = 42,
    kPhaseActuatorAlign = 70,
    kPhaseActuatorWait = 71,
    kPhasePressureProbe = 72,
    kPhasePressureEnter = 76,
    kPhasePressureWait = 77,
    kPhaseTableSize = 78,
    kPhaseDone = 99
};

// Levels of mUnknown124: digital only, analog, and analog with pressure-sensitive buttons.
constexpr int kReadyDigital = 1;
constexpr int kReadyAnalog = 2;
constexpr int kReadyPressure = 3;

// Layout of a report.
constexpr int kReportSize = 32;
enum ReportByte {
    kReportStatus = 0,
    kReportMode = 1,
    kReportButtonsHigh = 2,
    kReportButtonsLow = 3,
    kReportRightX = 4,
    kReportRightY = 5,
    kReportLeftX = 6,
    kReportLeftY = 7,
    kReportPressures = 8
};
constexpr int kReportOk = 0;
constexpr int kBitsPerByte = 8;
constexpr unsigned kButtonMask = 0xffff;

// The value an analog byte reads at rest.
constexpr int kAnalogCentre = 0x80;

// Entries of mUnknown130 and mUnknown134, one per analog axis.
enum Axis { kAxisLeftX = 0, kAxisLeftY = 1, kAxisRightX = 2, kAxisRightY = 3 };

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

// The setup step for the current phase.
inline void PadRecord::AdvancePhase(int nState) {
    switch (mPhase) {
    case kPhaseProbe: {
        if (nState != kPadStateStable && nState != kPadStateFindCtp1) {
            break;
        }
        int nId = scePadInfoMode(mPort, mSlot, kInfoModeCurrentId, 0);
        if (nId == 0) {
            break;
        }
        const int nExtendedId = scePadInfoMode(mPort, mSlot, kInfoModeCurrentExtendedId, 0);
        if (nExtendedId > 0) {
            nId = nExtendedId;
        }
        if (nId == kPadIdAnalog) {
            mUnknown124 = kReadyDigital;
            mPhase = kPhaseAnalogProbe;
        } else if (nId == kPadIdDualShock2) {
            mPhase = kPhaseActuatorAlign;
        } else {
            mPhase = kPhaseDone;
        }
        break;
    }
    case kPhaseAnalogProbe:
        if (scePadInfoMode(mPort, mSlot, kInfoModeCurrentExtendedId, 0) == 0) {
            mPhase = kPhaseDone;
            break;
        }
        ++mPhase;
        [[fallthrough]];
    case kPhaseAnalogSetMode:
        if (scePadSetMainMode(mPort, mSlot, kMainModeAnalog, kMainModeLock) == kPadCallSucceeded) {
            ++mPhase;
        }
        break;
    case kPhaseAnalogWait:
        if (scePadGetReqState(mPort, mSlot) == kReqStateFailed) {
            --mPhase;
        }
        if (scePadGetReqState(mPort, mSlot) == kReqStateComplete) {
            mPhase = kPhaseProbe;
            mUnknown124 = kReadyAnalog;
        }
        break;
    case kPhaseActuatorAlign:
        if (scePadInfoAct(mPort, mSlot, kInfoActCount, 0) == 0) {
            mPhase = kPhaseDone;
            break;
        }
        if (scePadSetActAlign(mPort, mSlot, mActAlign) == 0) {
            LogPrintf("BreugPad: Set actAlign failed!!!!!!!!!!!!\n");
            break;
        }
        ++mPhase;
        if (scePadGetReqState(mPort, mSlot) != kReqStateBusy) {
            LogPrintf("BreugPad: Set actAlign warning!!!!!!!!!!!!\n");
        }
        break;
    case kPhaseActuatorWait:
        if (scePadGetReqState(mPort, mSlot) == kReqStateFailed) {
            --mPhase;
        }
        if (scePadGetReqState(mPort, mSlot) == kReqStateComplete) {
            ++mPhase;
        }
        break;
    case kPhasePressureProbe:
        mPhase = scePadInfoPressMode(mPort, mSlot) == kPadCallSucceeded ? kPhasePressureEnter :
                                                                          kPhaseDone;
        break;
    case kPhasePressureEnter:
        if (scePadEnterPressMode(mPort, mSlot) == kPadCallSucceeded) {
            ++mPhase;
        }
        break;
    case kPhasePressureWait:
        if (scePadGetReqState(mPort, mSlot) == kReqStateFailed) {
            --mPhase;
        }
        if (scePadGetReqState(mPort, mSlot) == kReqStateComplete) {
            mPhase = kPhaseDone;
            mUnknown124 = kReadyPressure;
        }
        break;
    default:
        break;
    }
}

// 0x005bc998
int PadRecord::Read(unsigned int *pButtons,
                    unsigned char *pAxis0,
                    unsigned char *pAxis1,
                    unsigned char *pAxis2,
                    unsigned char *pAxis3,
                    unsigned char *pPressures,
                    short *pPressureDeltas) {
    int nLeftX = 0;
    int nLeftY = 0;
    int nRightX = 0;
    int nRightY = 0;
    ++mUnknown12c;
    const int nState = scePadGetState(mPort, mSlot);
    if (nState == kPadStateDisconnected) {
        mPhase = kPhaseProbe;
        mUnknown124 = 0;
    }
    if (static_cast<unsigned>(mPhase) < kPhaseTableSize) {
        AdvancePhase(nState);
    }

    if (nState != kPadStateStable && nState != kPadStateFindCtp1) {
        if (pButtons != nullptr) {
            *pButtons = mButtons;
        }
        if (pAxis0 != nullptr) {
            *pAxis0 = static_cast<unsigned char>(nLeftX);
        }
        if (pAxis1 != nullptr) {
            *pAxis1 = static_cast<unsigned char>(nLeftY);
        }
        if (pAxis2 != nullptr) {
            *pAxis2 = static_cast<unsigned char>(nRightX);
        }
        if (pAxis3 != nullptr) {
            *pAxis3 = static_cast<unsigned char>(nRightY);
        }
        return 0;
    }

    // With mUnknown124 at zero the report is never read, and the tail below still reads it.
    unsigned char abReport[kReportSize];
    if (mUnknown124 > 0) {
        mUnknown10c = mButtons;
        if (scePadRead(mPort, mSlot, abReport) == 0) {
            return 0;
        }
        const unsigned nNow =
            ~((abReport[kReportButtonsHigh] << kBitsPerByte) | abReport[kReportButtonsLow]) &
            kButtonMask;
        const unsigned nPrevious = static_cast<unsigned short>(mUnknown118);
        mUnknown118 = static_cast<short>(nNow);
        mUnknown108 ^= nNow & ~nPrevious;
        mUnknown104 |= static_cast<unsigned short>(mUnknown118);
        mButtons = static_cast<unsigned short>(mUnknown118);
    }

    if (mUnknown124 >= kReadyAnalog) {
        nRightX = abReport[kReportRightX] - kAnalogCentre;
        nRightY = abReport[kReportRightY] - kAnalogCentre;
        nLeftX = abReport[kReportLeftX] - kAnalogCentre;
        nLeftY = abReport[kReportLeftY] - kAnalogCentre;
        mUnknown134[kAxisLeftX] =
            static_cast<short>(nLeftX - static_cast<signed char>(mUnknown130[kAxisLeftX]));
        mUnknown134[kAxisLeftY] =
            static_cast<short>(nLeftY - static_cast<signed char>(mUnknown130[kAxisLeftY]));
        mUnknown134[kAxisRightX] =
            static_cast<short>(nRightX - static_cast<signed char>(mUnknown130[kAxisRightX]));
        mUnknown134[kAxisRightY] =
            static_cast<short>(nRightY - static_cast<signed char>(mUnknown130[kAxisRightY]));
        mUnknown130[kAxisLeftX] = static_cast<unsigned char>(nLeftX);
        mUnknown130[kAxisLeftY] = static_cast<unsigned char>(nLeftY);
        mUnknown130[kAxisRightX] = static_cast<unsigned char>(nRightX);
        mUnknown130[kAxisRightY] = static_cast<unsigned char>(nRightY);
        if (std::abs(nLeftX) < mUnknown128) {
            nLeftX = 0;
        }
        if (std::abs(nLeftY) < mUnknown128) {
            nLeftY = 0;
        }
        if (std::abs(nRightX) < mUnknown128) {
            nRightX = 0;
        }
        if (std::abs(nRightY) < mUnknown128) {
            nRightY = 0;
        }
    }

    if (pButtons != nullptr) {
        *pButtons = mButtons;
    }
    if (pAxis0 != nullptr) {
        *pAxis0 = static_cast<unsigned char>(nLeftX);
    }
    if (pAxis1 != nullptr) {
        *pAxis1 = static_cast<unsigned char>(nLeftY);
    }
    if (pAxis2 != nullptr) {
        *pAxis2 = static_cast<unsigned char>(nRightX);
    }
    if (pAxis3 != nullptr) {
        *pAxis3 = static_cast<unsigned char>(nRightY);
    }

    if (abReport[kReportStatus] == kReportOk && mUnknown124 == kReadyPressure) {
        for (int i = 0; i < kPressureByteCount; ++i) {
            const unsigned char nPressure = abReport[kReportPressures + i];
            if (pPressureDeltas != nullptr) {
                pPressureDeltas[i] = static_cast<short>(nPressure - mPressureBaseline[i]);
            }
            if (pPressures != nullptr) {
                pPressures[i] = nPressure;
            }
            mPressureBaseline[i] = nPressure;
        }
    }
    mUnknown120 = abReport[kReportMode];
    return mUnknown124;
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
