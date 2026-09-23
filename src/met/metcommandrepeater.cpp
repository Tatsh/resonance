#include "met/metcommandrepeater.h"

#include <vector>

#include "app/application.h"
#include "app/watchdog.h"
#include "profile/profiler.h"

namespace {

// The controllers the repeater serves.
constexpr int kControllerCount = 4;

// The pads are numbered from one, the records from zero.
constexpr int kFirstPad = 1;

// The delay before an armed command first repeats.
constexpr int kInitialDelayMs = 3000;

// The factor the panel's repeat scale is multiplied by to give the interval in milliseconds.
constexpr float kIntervalScaleMs = 50.0f;

// Nanoseconds in a millisecond, and half of one for rounding.
constexpr long long kNanosecondsPerMillisecond = 1000000;
constexpr long long kHalfMillisecondNs = kNanosecondsPerMillisecond / 2;

// Reading of the frame clock in nanoseconds, measured from the origin the watchdog's clock
// recorded when the run started. MetRenderer and MainLoop have their own copies of the same inline.
inline long long FrameClockNs(Watchdog *pWatchdog) {
    return (ProfileClockMilliseconds() - pWatchdog->mClock.mOriginMs) * kNanosecondsPerMillisecond;
}

// Milliseconds between two frame-clock readings, rounded rather than truncated.
inline int FrameIntervalMs(long long nNowNs, long long nThenNs) {
    return static_cast<int>((nNowNs - nThenNs + kHalfMillisecondNs) / kNanosecondsPerMillisecond);
}

} // namespace

// 0x002e54b0
MetCommandRepeater::MetCommandRepeater() {
    mRecords.insert(mRecords.begin(), kControllerCount, Record());
}

// 0x002e55b0
void MetCommandRepeater::Update(MetScreen *pPanel, const long long *pNowNanoseconds) {
    if (pPanel == nullptr) {
        return;
    }

    const int nIntervalMs = static_cast<int>(pPanel->mUnknown58 * kIntervalScaleMs);
    for (int nIndex = 0; nIndex < kControllerCount; ++nIndex) {
        Record &record = mRecords[nIndex];
        if (record.mCommand == 0) {
            continue;
        }
        if (record.mDelayMs > 0) {
            // Yes, the binary does not advance mLastNs while the delay runs.
            record.mDelayMs -= FrameIntervalMs(*pNowNanoseconds, record.mLastNs);
            continue;
        }
        if (FrameIntervalMs(*pNowNanoseconds, record.mLastNs) < nIntervalMs) {
            continue;
        }

        MetScreenCommand command;
        command.mCommand = record.mCommand;
        command.mPadIndex = nIndex + kFirstPad;
        command.mButton = record.mButton;
        pPanel->DeliverCommand(&command);
        mRecords[nIndex].mLastNs = FrameClockNs(Application::shared()->GetWatchdog());
    }
}

// 0x002e5790
void MetCommandRepeater::Arm(const MetScreenCommand *pCommand, int nButton, int nPadIndex) {
    const int nIndex = nPadIndex - kFirstPad;
    if (pCommand->mCommand == 0 && mRecords[nIndex].mButton != nButton) {
        return;
    }

    mRecords[nIndex].mCommand = 0;
    mRecords[nIndex].mButton = nButton;
    mRecords[nIndex].mCommand = pCommand->mCommand;
    mRecords[nIndex].mLastNs = FrameClockNs(Application::shared()->GetWatchdog());
    mRecords[nIndex].mDelayMs = kInitialDelayMs;
}

// 0x002e71c0
MetCommandRepeater::~MetCommandRepeater() {
}

// 0x002e7298
void MetCommandRepeater::Reset() {
    for (int nIndex = 0; nIndex < kControllerCount; ++nIndex) {
        mRecords[nIndex].mCommand = 0;
    }
}
