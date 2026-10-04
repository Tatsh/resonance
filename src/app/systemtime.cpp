#include "app/systemtime.h"

#include "os/cycles.h"

namespace {

// The factor Advance() scales its argument by before dividing by mNsPerUnit.
constexpr double kAdvanceScale = 1000000.0;

// Nanoseconds in one second. Divided by GetMillisecondsPerSecond(), it gives the nanoseconds in one
// clock unit.
constexpr double kNanosecondsPerSecond = 1000000000.0;

} // namespace

Sch::SystemTime::SystemTime() {
    mPausedRunMs = 0;
    mRunning = 1;
    mNsPerUnit = kNanosecondsPerSecond / static_cast<double>(GetMillisecondsPerSecond());
    const long long nNowMs = GetElapsedMilliseconds();
    mOriginMs = nNowMs;
    mStartMs = nNowMs;
}

void Sch::SystemTime::Mark(long long nNanoseconds) {
    const long long nMarkMs = static_cast<long long>(nNanoseconds / mNsPerUnit);
    if (mRunning != 0) {
        mStartMs = GetElapsedMilliseconds() - nMarkMs;
    } else {
        mPausedRunMs = nMarkMs;
    }
}

long long Sch::SystemTime::Now() {
    long long nRunMs;
    if (mRunning != 0) {
        nRunMs = GetElapsedMilliseconds() - mStartMs;
    } else {
        nRunMs = mPausedRunMs;
    }
    return static_cast<long long>(nRunMs * mNsPerUnit);
}

void Sch::SystemTime::Pause() {
    if (mRunning == 0) {
        return;
    }

    const long long nNowMs = GetElapsedMilliseconds();
    mRunning = 0;
    mPausedRunMs = nNowMs - mStartMs;
}

void Sch::SystemTime::Resume() {
    if (mRunning != 0) {
        return;
    }

    const long long nNowMs = GetElapsedMilliseconds();
    mRunning = 1;
    mStartMs = nNowMs - mPausedRunMs;
}

void Sch::SystemTime::Advance(int nAmount) {
    Pause();
    mPausedRunMs += static_cast<long long>(nAmount * kAdvanceScale / mNsPerUnit);
}
