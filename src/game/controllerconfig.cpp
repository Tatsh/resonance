#include "game/controllerconfig.h"

namespace {

// The record version Save() writes and Load() requires.
constexpr int kRecordVersion = 4;

// The default button code of each slot, in slot order. The binary stores the seventeen entries
// one by one.
constexpr int kDefaultButtons[] = {14, 16, 13, 15, 7, 4, 8, 1, 6, 2, 3, 101, 100, 102, 103, 11, 9};

// The two analog sticks' axis codes and press codes.
constexpr int kStickAFirstAxis = 100;
constexpr int kStickASecondAxis = 101;
constexpr int kStickBFirstAxis = 102;
constexpr int kStickBSecondAxis = 103;
constexpr int kStickAPress = 11;
constexpr int kStickBPress = 12;

// The codes ButtonCode() reports for the two stick choices.
constexpr int kChooseStickA = 500;
constexpr int kChooseStickB = 501;

// The code or index reported for a value with no translation.
constexpr int kNone = -1;

// The four-character action codes, in slot order.
constexpr int kRotateRight = 0x726f7452;  // `rotR`
constexpr int kRotateLeft = 0x726f744c;   // `rotL`
constexpr int kAdvance = 0x6164766e;      // `advn`
constexpr int kLoop = 0x6c6f6f70;         // `loop`
constexpr int kPitchRiff = 0x72706368;    // `rpch`
constexpr int kErase = 0x65726173;        // `eras`
constexpr int kAxisFX = 0x61786678;       // `axfx`
constexpr int kAxisRegister = 0x72656769; // `regi`
constexpr int kAxisX = 0x706f7778;        // `powx`
constexpr int kAxisY = 0x706f7779;        // `powy`
constexpr int kButtonPow = 0x706f7762;    // `powb`
constexpr int kPlayback = 0x7062636b;     // `pbck`

// Button indices the configuration screen offers, and the codes they stand for.
constexpr int kButtonCodes[] = {4, 1, 2, 3, 7, 5, 8, 6, kChooseStickA, kChooseStickB};
constexpr int kStickAIndex = 8;
constexpr int kStickBIndex = 9;

// The slot each configuration row controls.
constexpr int kRowSlots[] = {4, 5, 6, 7, 8, 9, 10, 12, 14};

// Whether a slot is one of the effect and register axes, or one of the power axes.
inline bool IsEffectAxisSlot(int nSlot) {
    return nSlot == ControllerConfig::kSlotAxisFX || nSlot == ControllerConfig::kSlotAxisRegister;
}

inline bool IsPowerAxisSlot(int nSlot) {
    return nSlot == ControllerConfig::kSlotAxisX || nSlot == ControllerConfig::kSlotAxisY;
}

} // namespace

// 0x00163a88
ControllerConfig::ControllerConfig() {
    mButtons.resize(kSlotCount);
    for (int i = 0; i < kSlotCount; ++i) {
        mButtons[i] = kDefaultButtons[i];
    }
}

// 0x00163c28
void ControllerConfig::SetButton(int nRow, int nButtonIndex) {
    int nSlot = ActionSlot(nRow);
    int nCode = ButtonCode(nButtonIndex);
    bool bStickAOnEffects;
    if (nCode == kChooseStickA) {
        if (IsEffectAxisSlot(nSlot)) {
            bStickAOnEffects = true;
        } else if (IsPowerAxisSlot(nSlot)) {
            bStickAOnEffects = false;
        } else {
            return;
        }
    } else if (nCode == kChooseStickB) {
        if (IsEffectAxisSlot(nSlot)) {
            bStickAOnEffects = false;
        } else if (IsPowerAxisSlot(nSlot)) {
            bStickAOnEffects = true;
        } else {
            return;
        }
    } else {
        mButtons[nSlot] = nCode;
        return;
    }

    if (bStickAOnEffects) {
        mButtons[kSlotAxisRegister] = kStickAFirstAxis;
        mButtons[kSlotAxisFX] = kStickASecondAxis;
        mButtons[kSlotAxisX] = kStickBFirstAxis;
        mButtons[kSlotAxisY] = kStickBSecondAxis;
        mButtons[kSlotButtonPow] = kStickAPress;
    } else {
        mButtons[kSlotAxisRegister] = kStickBFirstAxis;
        mButtons[kSlotAxisFX] = kStickBSecondAxis;
        mButtons[kSlotAxisX] = kStickAFirstAxis;
        mButtons[kSlotAxisY] = kStickASecondAxis;
        mButtons[kSlotButtonPow] = kStickBPress;
    }
}

// 0x00163d50
int ControllerConfig::ActionCode(int nSlot) {
    switch (nSlot) {
    case kSlotRotateRight:
        return kRotateRight;
    case kSlotRotateLeft:
        return kRotateLeft;
    case kSlotAdvance:
        return kAdvance;
    case kSlotLoop:
        return kLoop;
    case kSlotPitchRiffFirst:
    case kSlotPitchRiffFirst + 1:
    case kSlotPitchRiffFirst + 2:
    case kSlotPitchRiffFirst + 3:
    case kSlotPitchRiffFirst + 4:
    case kSlotPitchRiffFirst + 5:
        return kPitchRiff;
    case kSlotErase:
        return kErase;
    case kSlotAxisFX:
        return kAxisFX;
    case kSlotAxisRegister:
        return kAxisRegister;
    case kSlotAxisX:
        return kAxisX;
    case kSlotAxisY:
        return kAxisY;
    case kSlotButtonPow:
        return kButtonPow;
    case kSlotPlayback:
        return kPlayback;
    default:
        return 0;
    }
}

// 0x00163e10
void ControllerConfig::Load(IBStream &stream) {
    int nVersion;
    stream.Read(&nVersion, sizeof(nVersion));
    if (nVersion >= kRecordVersion) {
        int nCount;
        stream.Read(&nCount, sizeof(nCount));
        mButtons.resize(nCount);
        for (std::vector<int>::iterator it = mButtons.begin(); it != mButtons.end(); ++it) {
            stream.Read(&*it, sizeof(*it));
        }
    } else {
        // An older mapping is read and discarded.
        std::vector<int> discarded;
        int nCount;
        stream.Read(&nCount, sizeof(nCount));
        discarded.resize(nCount);
        for (std::vector<int>::iterator it = discarded.begin(); it != discarded.end(); ++it) {
            stream.Read(&*it, sizeof(*it));
        }
    }
}

// 0x00164b00
int ControllerConfig::GetButtonIndex(int nRow) {
    return ButtonIndex(mButtons[ActionSlot(nRow)]);
}

// 0x00164b40
int ControllerConfig::ButtonCode(int nButtonIndex) {
    if (static_cast<unsigned>(nButtonIndex) >= sizeof(kButtonCodes) / sizeof(kButtonCodes[0])) {
        return kNone;
    }
    return kButtonCodes[nButtonIndex];
}

// 0x00164bc0
int ControllerConfig::ButtonIndex(int nCode) {
    switch (nCode) {
    case kButtonCodes[0]:
        return 0;
    case kButtonCodes[1]:
        return 1;
    case kButtonCodes[2]:
        return 2;
    case kButtonCodes[3]:
        return 3;
    case kButtonCodes[4]:
        return 4;
    case kButtonCodes[5]:
        return 5;
    case kButtonCodes[6]:
        return 6;
    case kButtonCodes[7]:
        return 7;
    case kStickAFirstAxis:
    case kStickASecondAxis:
        return kStickAIndex;
    case kStickBFirstAxis:
    case kStickBSecondAxis:
        return kStickBIndex;
    default:
        return kNone;
    }
}

// 0x00164c40
int ControllerConfig::ActionSlot(int nRow) {
    if (static_cast<unsigned>(nRow) >= sizeof(kRowSlots) / sizeof(kRowSlots[0])) {
        return kNone;
    }
    return kRowSlots[nRow];
}

// 0x00164cb8
int ControllerConfig::RiffIndex(int nSlot) {
    switch (nSlot) {
    case kSlotPitchRiffFirst:
    case kSlotPitchRiffFirst + 1:
        return 0;
    case kSlotPitchRiffFirst + 2:
    case kSlotPitchRiffFirst + 3:
        return 1;
    case kSlotPitchRiffFirst + 4:
    case kSlotPitchRiffFirst + 5:
        return 2;
    default:
        return 0;
    }
}

// 0x00164d00
void ControllerConfig::Save(OBStream &stream) {
    int nVersion = kRecordVersion;
    stream.Write(&nVersion, sizeof(nVersion));
    int nCount = mButtons.size();
    stream.Write(&nCount, sizeof(nCount));
    for (std::vector<int>::iterator it = mButtons.begin(); it != mButtons.end(); ++it) {
        int nCode = *it;
        stream.Write(&nCode, sizeof(nCode));
    }
}

// 0x00164dd0
ControllerConfig &ControllerConfig::operator=(const ControllerConfig &other) {
    mButtons = other.mButtons;
    return *this;
}
