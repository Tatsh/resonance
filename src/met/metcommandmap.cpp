#include "met/metcommandmap.h"

#include <map>
#include <vector>

namespace {

// The four-character tags of a keyboard and a joypad reading.
constexpr int kKeyTag = 0x6b657920; // 'key '
constexpr int kJoyTag = 0x6a6f7920; // 'joy '

// The command a pressed key yields.
constexpr int kKeyCommand = 15;

// The held-direction records, one per controller index.
constexpr std::vector<std::map<int, int> >::size_type kControllerCount = 5;

// The codes joypad buttons 1 through 16 yield while pressed, indexed by the button less one.
const int kButtonCommands[] = {6, 7, 5, 8, 12, 14, 11, 13, 9, 10, 21, 20, 1, 4, 2, 3};
constexpr int kFirstButton = 1;
constexpr int kLastButton = 16;

// The four analogue directions and the codes each yields at its two extremes.
constexpr int kLeftStickX = 100;
constexpr int kLeftStickY = 101;
constexpr int kRightStickX = 102;
constexpr int kRightStickY = 103;
constexpr int kRightStickXNegative = 0x10;
constexpr int kRightStickXPositive = 0x11;
constexpr int kRightStickYNegative = 0x13;
constexpr int kRightStickYPositive = 0x12;

// What an unmapped button yields, and the pad index it records with it.
constexpr int kNoCommand = -1;
constexpr int kUnmappedPadIndex = 1;

// The released command, and the thresholds either side of an axis's centre.
constexpr int kReleased = 0;
constexpr float kAxisLow = 0.1f;
constexpr float kAxisHigh = 0.9f;

// The value a held-direction record stores while its direction is held.
constexpr int kHeld = 1;

} // namespace

// 0x002e33f0
MetCommandMap::MetCommandMap() {
    mHeld.resize(kControllerCount, std::map<int, int>());
}

// 0x002e3738
int MetCommandMap::Translate(const MetControllerReading *pReading, MetScreenCommand *pCommand) {
    pCommand->mPadIndex = pReading->mPadIndex;
    if (pReading->mTag == kKeyTag && pReading->mValue > 0.0f) {
        pCommand->mCommand = kKeyCommand;
        pCommand->mButton = pReading->mButton;
    } else if (pReading->mTag == kJoyTag) {
        const int nButton = pReading->mButton;
        if (nButton >= kFirstButton && nButton <= kLastButton) {
            pCommand->mCommand =
                (pReading->mValue != 0.0f) ? kButtonCommands[nButton - kFirstButton] : kReleased;
        } else if (nButton == kLeftStickX) {
            pCommand->mCommand = AxisCommand(nButton,
                                             pCommand->mPadIndex,
                                             pReading->mValue,
                                             kMetScreenCommandLeft,
                                             kMetScreenCommandRight);
        } else if (nButton == kLeftStickY) {
            pCommand->mCommand = AxisCommand(nButton,
                                             pCommand->mPadIndex,
                                             pReading->mValue,
                                             kMetScreenCommandPrevious,
                                             kMetScreenCommandNext);
        } else if (nButton == kRightStickX) {
            pCommand->mCommand = AxisCommand(nButton,
                                             pCommand->mPadIndex,
                                             pReading->mValue,
                                             kRightStickXNegative,
                                             kRightStickXPositive);
        } else if (nButton == kRightStickY) {
            pCommand->mCommand = AxisCommand(nButton,
                                             pCommand->mPadIndex,
                                             pReading->mValue,
                                             kRightStickYNegative,
                                             kRightStickYPositive);
        } else {
            // Yes, the binary overwrites the pad index for an unmapped button.
            pCommand->mPadIndex = kUnmappedPadIndex;
            pCommand->mCommand = kNoCommand;
        }
    }
    return pCommand->mCommand != kNoCommand;
}

// 0x002e3ac0
int MetCommandMap::AxisCommand(
    int nButton, int nPadIndex, float flValue, int nNegative, int nPositive) {
    if (flValue < kAxisLow) {
        if (mHeld[nPadIndex][nButton] == kHeld) {
            return kNoCommand;
        }
        mHeld[nPadIndex][nButton] = kHeld;
        return nNegative;
    }
    // The binary widens both sides to double before comparing.
    if (static_cast<double>(flValue) > static_cast<double>(kAxisHigh)) {
        if (mHeld[nPadIndex][nButton] == kHeld) {
            return kNoCommand;
        }
        mHeld[nPadIndex][nButton] = kHeld;
        return nPositive;
    }
    mHeld[nPadIndex][nButton] = kReleased;
    return kReleased;
}
