#include <iostream>

#include "app/activefilter.h"
#include "app/filterlover.h"
#include "mid/mbt.h"
#include "sch/command.h"
#include "sch/tickclock.h"

// The original file is AppActiveFilter.cpp. Its static initialiser at 0x00100040, run through
// 0x00100350, registers the command below with identifier zero through the registrar Sch::Command
// records as unreconstructed.

namespace {

// The handle value of a command the clock has not queued yet.
constexpr int kUnallocatedCommand = -2;

constexpr float kDefaultRetention = 0.5f;

// Whether ActiveFilter::Update() asks the clock to record its post.
constexpr int kNotRecordable = 0;

/**
 * Scheduler command that runs ActiveFilter::Update().
 *
 * `Q236_GLOBAL_$N$AppActiveFilter.cpp1ZOhgb3Cmd` in the RTTI, with Sch::Command as its one base
 * and its vtable at `0x007cc688`. ActiveFilter::SetTarget() expands the constructor into its
 * 0x14-byte allocation.
 *
 * The destructor at `0x00100280` is implicitly declared.
 */
class Cmd : public Sch::Command {
public:
    explicit Cmd(ActiveFilter *pOwner) : mOwner(pOwner) {
    }

    // 0x001002f8
    virtual int CmdID() {
        return sCmdID;
    }

    // 0x00100308
    virtual void Execute() {
        mOwner->Update();
    }

    // 0x00100328
    virtual void Print(std::ostream &stream) {
        stream << "{ActiveFilter}";
    }

    // The word at 0x00666ea8, which the image initialises to zero.
    static int sCmdID;

private:
    ActiveFilter *mOwner; // +0x0c
    // Default-constructed to kMBTInfinity and never read.
    Mid::MBT mUnused; // +0x10
};

int Cmd::sCmdID;

} // namespace

// 0x00100080
ActiveFilter::ActiveFilter(Sch::TickClock *pClock, FilterLover *pLover)
    : mTarget(0.0f), mRetention(kDefaultRetention), mValue(0.0f), mLover(pLover), mClock(pClock),
      mStepCommand(nullptr) {
    mCommand.mValue = kUnallocatedCommand;
    mInterval.mValue = 0;
}

// 0x00100100
ActiveFilter::~ActiveFilter() {
    const CmdID command = mCommand;
    mClock->Withdraw(command);
    if (mStepCommand != nullptr) {
        mStepCommand->Release();
    }
    mStepCommand = nullptr;
}

// 0x00100150
void ActiveFilter::SetTarget(float flTarget) {
    mTarget = flTarget;
    if (mStepCommand != nullptr) {
        return;
    }

    mStepCommand = new Cmd(this);
    mValue = mTarget;
    Update();
}

// 0x001001c8
void ActiveFilter::Update() {
    mValue = (1.0 - mRetention) * mTarget + mRetention * mValue;
    mLover->OnFilterValue(mValue);
    mClock->PostIn(mStepCommand, mInterval, mCommand, kNotRecordable);
}
