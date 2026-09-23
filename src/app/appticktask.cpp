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
// `Cmd` in the anonymous namespace of AppTickTask.cpp, which the RTTI records. Its table is at
// 0x007d3080 and retains Sch::Command::Save() and Load().
class Cmd : public Sch::Command {
public:
    explicit Cmd(TickTask *pTask) : mTask(pTask) {
        if (mTask != nullptr) {
            ++mTask->mRefs;
        }
    }

    // 0x0013acd0
    virtual ~Cmd() {
        if (mTask != nullptr) {
            mTask->Release();
        }
    }

    // 0x0013ad80. Nothing registers the factory, and it produces nothing.
    static Sch::Command *New() {
        return nullptr;
    }

    // 0x0013acc0
    virtual int CmdID() {
        return sCmdID;
    }

    // 0x0013ad30
    virtual void Execute() {
        mTask->Run();
    }

    // 0x0013ad50
    virtual void Print(std::ostream &stream) {
        mTask->Print(stream);
    }

    // 0x006718f8. The image initialises it to zero.
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

// 0x0013ad88
TickTask::TickTask(Sch::TickClock *pClock, int nPeriod, int bAligned)
    : mClock(pClock), mPeriod(nPeriod), mNextTick(kMBTInfinity), mEpoch(kMBTInfinity),
      mAligned(bAligned) {
    mCommand.mValue = kUnallocatedCommand;
}

// 0x0013adc8
TickTask::~TickTask() {
    Stop();
}

// 0x0013ac48
void TickTask::Print(std::ostream &stream) {
    stream << kDescription;
}

// 0x0013a860
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

// 0x0013aa38
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

// 0x0013ae10
void TickTask::Stop() {
    const CmdID command = mCommand;
    mClock->Withdraw(command);
    mCommand.mValue = kUnallocatedCommand;
}
