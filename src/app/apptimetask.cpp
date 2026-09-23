#include <iostream>

#include "app/timetask.h"
#include "sch/command.h"
#include "sch/tick.h"
#include "sch/tickclock.h"

namespace {

// The handle a task starts with, before any command is posted.
constexpr int kUnallocatedCommand = -2;

// Run() posts an unrecorded, absolute command.
constexpr int kNotRecordable = 0;
constexpr int kAbsolute = 0;

constexpr char kDescription[] = "{TimeTask}";

// Scheduler command that runs one TimeTask, holding a reference on it.
//
// `Cmd` in the anonymous namespace of AppTimeTask.cpp, which the RTTI records. Its table is at
// 0x007d3160 and retains Sch::Command::Save() and Load().
class Cmd : public Sch::Command {
public:
    explicit Cmd(TimeTask *pTask) : mTask(pTask) {
        if (mTask != nullptr) {
            ++mTask->mRefs;
        }
    }

    // 0x0013b048
    virtual ~Cmd() {
        if (mTask != nullptr) {
            mTask->Release();
        }
    }

    // 0x0013b038
    virtual int CmdID() {
        return sCmdID;
    }

    // 0x0013b0a8
    virtual void Execute() {
        mTask->Run();
    }

    // 0x0013b0c8
    virtual void Print(std::ostream &stream) {
        mTask->Print(stream);
    }

    // 0x00671b60. The image initialises it to zero.
    static int sCmdID;

private:
    TimeTask *mTask; // +0x0c
};

int Cmd::sCmdID;

} // namespace

// 0x0013b100
TimeTask::TimeTask(Sch::TickClock *pClock, long long nPeriodNs)
    : mClock(pClock), mPeriodNs(nPeriodNs), mNextNs(0), mEpochNs(0) {
    mCommand.mValue = kUnallocatedCommand;
}

// 0x0013b138
TimeTask::~TimeTask() {
    Stop();
}

// 0x0013afc0
void TimeTask::Print(std::ostream &stream) {
    stream << kDescription;
}

// 0x0013b180
void TimeTask::Start(long long nEpochOffsetNs) {
    mNextNs = mClock->Now();
    if (nEpochOffsetNs == kNoEpochOffset) {
        mEpochNs = 0;
    } else {
        mEpochNs = mNextNs - nEpochOffsetNs;
    }
    Run();
}

// 0x0013ae70
void TimeTask::Run() {
    if (Tick(mNextNs - mEpochNs) != 1) {
        return;
    }

    mNextNs += mPeriodNs;
    Cmd *pCommand = new Cmd(this);
    const Sch::Tick due{mNextNs};
    mClock->Post(pCommand, due, mCommand, kNotRecordable, kAbsolute);
    if (pCommand != nullptr) {
        pCommand->Release();
    }
}

// 0x0013b1f0
void TimeTask::Stop() {
    const CmdID command = mCommand;
    mClock->Withdraw(command);
    mCommand.mValue = kUnallocatedCommand;
}
