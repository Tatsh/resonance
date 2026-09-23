#include "game/gsperiodical.h"

#include <algorithm>
#include <iostream>

#include "game/phrasemaker.h"
#include "mid/mbt.h"
#include "sch/command.h"
#include "sch/tickclock.h"

namespace {

// The handle value of a command the clock has not queued yet.
constexpr int kUnallocatedCommand = -2;

// The clamp the inline Mid::MBT arithmetic applies to a computed position.
inline int ClampPosition(int nTick) {
    return std::min(std::max(nTick, kMBTMinimum), kMBTMaximum);
}

/**
 * Scheduler command that runs one GsPeriodical at one song position.
 *
 * `Q235_GLOBAL_$N$GsPeriodical.cppdKuhgb13PeriodicalCmd` in the RTTI, with Sch::Command as its one
 * base. Its vtable at `0x007e1908` retains Sch::Command::Save() and Load(). GsPeriodical::PostAt()
 * expands the constructor into its 0x14-byte allocation.
 *
 * The destructor at `0x001b4798` is implicitly declared. It stores the base table pointer and runs
 * Attachment's destructor, which is what the compiler generates.
 */
class PeriodicalCmd : public Sch::Command {
public:
    PeriodicalCmd(GsPeriodical *pOwner, int nTick) : mOwner(pOwner), mTick(nTick) {
    }

    // 0x001b4810
    virtual int CmdID() {
        return sCmdID;
    }

    // 0x001b4820
    virtual void Execute() {
        mOwner->Run(mTick);
    }

    // 0x001b4840
    virtual void Print(std::ostream &stream) {
        stream << "{Periodical}";
    }

    // The word at 0x006889e0, which the image initialises to zero.
    static int sCmdID;

private:
    GsPeriodical *mOwner; // +0x0c
    int mTick;            // +0x10
};

int PeriodicalCmd::sCmdID;

} // namespace

// 0x001b4738
GsPeriodical::GsPeriodical(Sch::TickClock *pClock, PhraseMaker *pPhraseMaker, int nPeriod)
    : mOrigin(kMBTInfinity), mPeriod(nPeriod), mClock(pClock), mPhraseMaker(pPhraseMaker) {
    mCommand.mValue = kUnallocatedCommand;
    mOrigin = pPhraseMaker->Slot5();
}

// 0x001b4548
void GsPeriodical::PostAt(int nTick) {
    PeriodicalCmd *pCommand = new PeriodicalCmd(this, nTick);
    mClock->PostAtSongTick(pCommand, nTick, mCommand);
    if (pCommand != nullptr) {
        pCommand->Release();
    }
}

// 0x001b45d0
void GsPeriodical::Run(int nTick) {
    const Mid::MBT offset(ClampPosition(nTick - mOrigin));
    mPhraseMaker->Slot4(offset.mTick / mPeriod);

    const Mid::MBT next(ClampPosition(nTick + mPeriod));
    PostAt(next.mTick);
}

// 0x001b4870
void GsPeriodical::Post() {
    const Mid::MBT first(ClampPosition(mOrigin + mPeriod));
    PostAt(first.mTick);
}

// 0x001b48f8
void GsPeriodical::Withdraw() {
    const CmdID command = mCommand;
    mClock->Withdraw(command);
}
