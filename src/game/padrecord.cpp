#include "game/padrecord.h"

#include <cstdlib>
#include <libpad.h>

#include "os/log.h"

namespace {

// Pad states scePadGetState() reports.
constexpr int kPadStateDisconnected = 0;
constexpr int kPadStateFindCtp1 = 2;
constexpr int kPadStateStable = 6;

// Terms scePadInfoMode() takes, and the identifiers BreugPadRead() recognises.
constexpr int kInfoModeCurrentId = 1;
constexpr int kInfoModeCurrentExtendedId = 2;
constexpr int kPadIdAnalog = 4;
constexpr int kPadIdDualShock2 = 7;

// Progress scePadGetReqState() reports.
constexpr int kReqStateComplete = 0;
constexpr int kReqStateFailed = 1;
constexpr int kReqStateBusy = 2;

// The main mode BreugPadRead() requests, analog and locked.
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

// Levels of mReadyLevel: digital only, analog, and analog with pressure-sensitive buttons.
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

// Entries of mLastAxes and mAxisDeltas, one per analog axis.
enum Axis { kAxisLeftX = 0, kAxisLeftY = 1, kAxisRightX = 2, kAxisRightY = 3 };

// Byte of the unused alignment entries.
constexpr unsigned char kActuatorUnused = 0xff;

// Report bytes the first two actuators take, which leaves the other four unused.
constexpr unsigned char kSmallMotorByte = 0;
constexpr unsigned char kBigMotorByte = 1;

// Actuator entries of the small and big motors.
enum ActuatorIndex { kSmallMotor = 0, kBigMotor = 1 };

// mReadyLevel from which BreugPadSetMotors() drives the motors.
constexpr int kVibrationReady = 2;

// The setup step for the current phase. BreugPadRead() expands it inline, and it has no separate
// address.
inline void AdvancePhase(PadRecord *pPad, int nState) {
    switch (pPad->mPhase) {
    case kPhaseProbe: {
        if (nState != kPadStateStable && nState != kPadStateFindCtp1) {
            break;
        }
        int nId = scePadInfoMode(pPad->mPort, pPad->mSlot, kInfoModeCurrentId, 0);
        if (nId == 0) {
            break;
        }
        const int nExtendedId =
            scePadInfoMode(pPad->mPort, pPad->mSlot, kInfoModeCurrentExtendedId, 0);
        if (nExtendedId > 0) {
            nId = nExtendedId;
        }
        if (nId == kPadIdAnalog) {
            pPad->mReadyLevel = kReadyDigital;
            pPad->mPhase = kPhaseAnalogProbe;
        } else if (nId == kPadIdDualShock2) {
            pPad->mPhase = kPhaseActuatorAlign;
        } else {
            pPad->mPhase = kPhaseDone;
        }
        break;
    }
    case kPhaseAnalogProbe:
        if (scePadInfoMode(pPad->mPort, pPad->mSlot, kInfoModeCurrentExtendedId, 0) == 0) {
            pPad->mPhase = kPhaseDone;
            break;
        }
        ++pPad->mPhase;
        [[fallthrough]];
    case kPhaseAnalogSetMode:
        if (scePadSetMainMode(pPad->mPort, pPad->mSlot, kMainModeAnalog, kMainModeLock) ==
            kPadCallSucceeded) {
            ++pPad->mPhase;
        }
        break;
    case kPhaseAnalogWait:
        if (scePadGetReqState(pPad->mPort, pPad->mSlot) == kReqStateFailed) {
            --pPad->mPhase;
        }
        if (scePadGetReqState(pPad->mPort, pPad->mSlot) == kReqStateComplete) {
            pPad->mPhase = kPhaseProbe;
            pPad->mReadyLevel = kReadyAnalog;
        }
        break;
    case kPhaseActuatorAlign:
        if (scePadInfoAct(pPad->mPort, pPad->mSlot, kInfoActCount, 0) == 0) {
            pPad->mPhase = kPhaseDone;
            break;
        }
        if (scePadSetActAlign(pPad->mPort, pPad->mSlot, pPad->mActAlign) == 0) {
            printf("BreugPad: Set actAlign failed!!!!!!!!!!!!\n");
            break;
        }
        ++pPad->mPhase;
        if (scePadGetReqState(pPad->mPort, pPad->mSlot) != kReqStateBusy) {
            printf("BreugPad: Set actAlign warning!!!!!!!!!!!!\n");
        }
        break;
    case kPhaseActuatorWait:
        if (scePadGetReqState(pPad->mPort, pPad->mSlot) == kReqStateFailed) {
            --pPad->mPhase;
        }
        if (scePadGetReqState(pPad->mPort, pPad->mSlot) == kReqStateComplete) {
            ++pPad->mPhase;
        }
        break;
    case kPhasePressureProbe:
        pPad->mPhase = scePadInfoPressMode(pPad->mPort, pPad->mSlot) == kPadCallSucceeded ?
                           kPhasePressureEnter :
                           kPhaseDone;
        break;
    case kPhasePressureEnter:
        if (scePadEnterPressMode(pPad->mPort, pPad->mSlot) == kPadCallSucceeded) {
            ++pPad->mPhase;
        }
        break;
    case kPhasePressureWait:
        if (scePadGetReqState(pPad->mPort, pPad->mSlot) == kReqStateFailed) {
            --pPad->mPhase;
        }
        if (scePadGetReqState(pPad->mPort, pPad->mSlot) == kReqStateComplete) {
            pPad->mPhase = kPhaseDone;
            pPad->mReadyLevel = kReadyPressure;
        }
        break;
    default:
        break;
    }
}

} // namespace

// NTSC-U/C: 0x005bcf58, PAL: 0x00585f40
void BreugPadInit(PadRecord *pPad, int nPort, int nSlot, int nDeadZone) {
    // NTSC-U/C: 0x00777fbc, PAL: 0x0076560c
    static int firstPadStarted;
    for (int i = 0; i < PadRecord::kActuatorByteCount; ++i) {
        pPad->mActDirect[i] = 0;
        pPad->mActAlign[i] = kActuatorUnused;
    }
    for (int i = PadRecord::kPressureByteCount - 1; i >= 0; --i) {
        pPad->mPressureBaseline[i] = 0;
    }
    pPad->mActAlign[kSmallMotor] = kSmallMotorByte;
    pPad->mActAlign[kBigMotor] = kBigMotorByte;
    pPad->mPort = nPort;
    pPad->mSlot = nSlot;
    pPad->mPhase = 0;
    pPad->mRawButtons = 0;
    pPad->mReadCount = 0;
    pPad->mReportMode = 0;
    if (firstPadStarted == 0) {
        scePadInit(0);
        firstPadStarted = 1;
    }
    for (auto &byte : pPad->mLastAxes) {
        byte = 0;
    }
    for (auto &value : pPad->mAxisDeltas) {
        value = 0;
    }
    scePadPortOpen(nPort, nSlot, pPad->mDmaArea);
    pPad->mDeadZone = nDeadZone;
    pPad->mButtons = 0;
    pPad->mHeldButtonsSeen = 0;
    pPad->mToggledButtons = 0;
    pPad->mPreviousButtons = 0;
}

// NTSC-U/C: 0x005bc998, PAL: 0x00585960
int BreugPadRead(PadRecord *pPad,
                 unsigned int *pButtons,
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
    ++pPad->mReadCount;
    const int nState = scePadGetState(pPad->mPort, pPad->mSlot);
    if (nState == kPadStateDisconnected) {
        pPad->mPhase = kPhaseProbe;
        pPad->mReadyLevel = 0;
    }
    if (static_cast<unsigned>(pPad->mPhase) < kPhaseTableSize) {
        AdvancePhase(pPad, nState);
    }

    if (nState != kPadStateStable && nState != kPadStateFindCtp1) {
        if (pButtons != nullptr) {
            *pButtons = pPad->mButtons;
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

    // With mReadyLevel at zero the report is never read, and the tail below still reads it.
    unsigned char abReport[kReportSize];
    if (pPad->mReadyLevel > 0) {
        pPad->mPreviousButtons = pPad->mButtons;
        if (scePadRead(pPad->mPort, pPad->mSlot, abReport) == 0) {
            return 0;
        }
        const unsigned nNow =
            ~((abReport[kReportButtonsHigh] << kBitsPerByte) | abReport[kReportButtonsLow]) &
            kButtonMask;
        const unsigned nPrevious = static_cast<unsigned short>(pPad->mRawButtons);
        pPad->mRawButtons = static_cast<short>(nNow);
        pPad->mToggledButtons ^= nNow & ~nPrevious;
        pPad->mHeldButtonsSeen |= static_cast<unsigned short>(pPad->mRawButtons);
        pPad->mButtons = static_cast<unsigned short>(pPad->mRawButtons);
    }

    if (pPad->mReadyLevel >= kReadyAnalog) {
        nRightX = abReport[kReportRightX] - kAnalogCentre;
        nRightY = abReport[kReportRightY] - kAnalogCentre;
        nLeftX = abReport[kReportLeftX] - kAnalogCentre;
        nLeftY = abReport[kReportLeftY] - kAnalogCentre;
        pPad->mAxisDeltas[kAxisLeftX] =
            static_cast<short>(nLeftX - static_cast<signed char>(pPad->mLastAxes[kAxisLeftX]));
        pPad->mAxisDeltas[kAxisLeftY] =
            static_cast<short>(nLeftY - static_cast<signed char>(pPad->mLastAxes[kAxisLeftY]));
        pPad->mAxisDeltas[kAxisRightX] =
            static_cast<short>(nRightX - static_cast<signed char>(pPad->mLastAxes[kAxisRightX]));
        pPad->mAxisDeltas[kAxisRightY] =
            static_cast<short>(nRightY - static_cast<signed char>(pPad->mLastAxes[kAxisRightY]));
        pPad->mLastAxes[kAxisLeftX] = static_cast<unsigned char>(nLeftX);
        pPad->mLastAxes[kAxisLeftY] = static_cast<unsigned char>(nLeftY);
        pPad->mLastAxes[kAxisRightX] = static_cast<unsigned char>(nRightX);
        pPad->mLastAxes[kAxisRightY] = static_cast<unsigned char>(nRightY);
        if (std::abs(nLeftX) < pPad->mDeadZone) {
            nLeftX = 0;
        }
        if (std::abs(nLeftY) < pPad->mDeadZone) {
            nLeftY = 0;
        }
        if (std::abs(nRightX) < pPad->mDeadZone) {
            nRightX = 0;
        }
        if (std::abs(nRightY) < pPad->mDeadZone) {
            nRightY = 0;
        }
    }

    if (pButtons != nullptr) {
        *pButtons = pPad->mButtons;
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

    if (abReport[kReportStatus] == kReportOk && pPad->mReadyLevel == kReadyPressure) {
        for (int i = 0; i < PadRecord::kPressureByteCount; ++i) {
            const unsigned char nPressure = abReport[kReportPressures + i];
            if (pPressureDeltas != nullptr) {
                pPressureDeltas[i] = static_cast<short>(nPressure - pPad->mPressureBaseline[i]);
            }
            if (pPressures != nullptr) {
                pPressures[i] = nPressure;
            }
            pPad->mPressureBaseline[i] = nPressure;
        }
    }
    pPad->mReportMode = abReport[kReportMode];
    return pPad->mReadyLevel;
}

// NTSC-U/C: 0x005bd0b0, PAL: 0x00586098
void BreugPadSetMotors(PadRecord *pPad, int nSmallMotor, int nBigMotor) {
    if (pPad->mReadyLevel < kVibrationReady) {
        return;
    }
    pPad->mActDirect[kSmallMotor] = nSmallMotor > 0;
    pPad->mActDirect[kBigMotor] = nBigMotor;
    scePadSetActDirect(pPad->mPort, pPad->mSlot, pPad->mActDirect);
}

#ifdef VIDEO_STANDARD_PAL
// PAL: 0x00585f20
void PadRecord::EndLibrary() {
    scePadEnd();
}
#endif
