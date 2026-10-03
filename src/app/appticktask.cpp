#include <algorithm>
#include <iostream>

#include "app/ticktask.h"
#include "sch/command.h"

namespace {

// The handle a task starts with, before any command is posted.
constexpr int kUnallocatedCommand = -2;

constexpr char kDescription[] = "{TickTask}";

// Scheduler command that runs one TickTask, holding a reference on it.
//
// `Cmd` in the anonymous namespace of AppTickTask.cpp, which the RTTI records. Its table is
// at 0x007d3080 and retains Sch::Command::saveGuts() and restoreGuts().
class Cmd : public Sch::Command {
public:
    explicit Cmd(TickTask *pTask) : mTask(pTask) {
        if (mTask != nullptr) {
            ++mTask->mRefs;
        }
    }

    // NTSC-U/C: 0x0013acd0, PAL: 0x0013b618
    virtual ~Cmd() {
        if (mTask != nullptr) {
            mTask->Release();
        }
    }

    // NTSC-U/C: 0x0013ad80, PAL: 0x0013b6c8
    // Nothing registers the factory, and it produces nothing.
    static Sch::Command *New() {
        return nullptr;
    }

    // NTSC-U/C: 0x0013acc0, PAL: 0x0013b608
    virtual int CmdID() {
        return sCmdID;
    }

    // NTSC-U/C: 0x0013ad30, PAL: 0x0013b678
    virtual void Execute() {
        mTask->Run();
    }

    // NTSC-U/C: 0x0013ad50, PAL: 0x0013b698
    virtual void Print(std::ostream &stream) {
        mTask->Print(stream);
    }

    // The image initialises it to zero.
    // NTSC-U/C: 0x006718f8, PAL: 0x006b2500
    static int sCmdID;

private:
    TickTask *mTask; // +0x0c
};

int Cmd::sCmdID;

// Saturate a tick against Mid::MBT's infinity bounds.
inline int ClampTick(int nTick) {
    return std::min(std::max(nTick, kMBTMinimum), kMBTMaximum);
}

} // namespace

// NTSC-U/C: 0x0013ad88, PAL: 0x0013b6d0
TickTask::TickTask(Sch::TickClock *pClock, int nPeriod, int bAligned)
    : mClock(pClock), mPeriod(nPeriod), mNextTick(kMBTInfinity), mEpoch(kMBTInfinity),
      mAligned(bAligned) {
    mCommand.mValue = kUnallocatedCommand;
}

// NTSC-U/C: 0x0013adc8, PAL: 0x0013b710
TickTask::~TickTask() {
    Stop();
}

// NTSC-U/C: 0x0013ac48, PAL: 0x0013b590
void TickTask::Print(std::ostream &stream) {
    stream << kDescription;
}

// NTSC-U/C: 0x0013a860, PAL: 0x0013b1a8
void TickTask::Start(int nEpochOffset) {
    mNextTick = mClock->SongTick();
    if (nEpochOffset == kMBTInfinity) {
        mEpoch = Mid::MBT(0).mTick;
    } else {
        mEpoch = Mid::MBT(ClampTick(mNextTick - nEpochOffset)).mTick;
    }

    if (mAligned == 0) {
        Run();
        return;
    }

    const int nNext = Mid::MBT(ClampTick(mNextTick + mPeriod)).mTick;
    mNextTick = Mid::MBT(ClampTick((nNext / mPeriod) * mPeriod)).mTick;
    Cmd *pCommand = new Cmd(this);
    mClock->PostAtSongTick(pCommand, mNextTick, mCommand);
    if (pCommand != nullptr) {
        pCommand->Release();
    }
}

// NTSC-U/C: 0x0013aa38, PAL: 0x0013b380
void TickTask::Run() {
    const int nElapsed = Mid::MBT(ClampTick(mNextTick - mEpoch)).mTick;
    if (Tick(nElapsed) != 1) {
        return;
    }

    mNextTick = ClampTick(mNextTick + mPeriod);
    Cmd *pCommand = new Cmd(this);
    mClock->PostAtSongTick(pCommand, mNextTick, mCommand);
    if (pCommand != nullptr) {
        pCommand->Release();
    }
}

// NTSC-U/C: 0x0013ae10, PAL: 0x0013b758
void TickTask::Stop() {
    const CmdID command = mCommand;
    mClock->Withdraw(command);
    mCommand.mValue = kUnallocatedCommand;
}
