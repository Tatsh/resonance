#include <iostream>

#include "app/timeclock.h"
#include "app/timetask.h"
#include "sch/command.h"
#include "sch/time.h"

namespace {

// The handle a task starts with, before any command is posted.
constexpr int kUnallocatedCommand = -2;

// Run() posts an unrecorded, absolute command.
constexpr int kNotRecordable = 0;
constexpr int kAbsolute = 0;

constexpr char kDescription[] = "{TimeTask}";

// Scheduler command that runs one TimeTask, holding a reference on it.
//
// `Cmd` in the anonymous namespace of AppTimeTask.cpp, which the RTTI records. Its table is
// at 0x007d3160 and retains Sch::Command::saveGuts() and restoreGuts().
class Cmd : public Sch::Command {
public:
    explicit Cmd(TimeTask *pTask) : mTask(pTask) {
        if (mTask != nullptr) {
            ++mTask->mRefs;
        }
    }

    // NTSC-U/C: 0x0013b048, PAL: 0x0013b990
    virtual ~Cmd() {
        if (mTask != nullptr) {
            mTask->Release();
        }
    }

    // NTSC-U/C: 0x0013b038, PAL: 0x0013b980
    virtual int CmdID() {
        return sCmdID;
    }

    // NTSC-U/C: 0x0013b0a8, PAL: 0x0013b9f0
    virtual void Execute() {
        mTask->Run();
    }

    // NTSC-U/C: 0x0013b0c8, PAL: 0x0013ba10
    virtual void Print(std::ostream &stream) {
        mTask->Print(stream);
    }

    // The image initialises it to zero.
    // NTSC-U/C: 0x00671b60, PAL: 0x006b2768
    static int sCmdID;

private:
    TimeTask *mTask; // +0x0c
};

int Cmd::sCmdID;

} // namespace

TimeTask::TimeTask(Sch::TimeClock *pClock, long long nPeriodNs)
    : mClock(pClock), mPeriodNs(nPeriodNs), mNextNs(0), mEpochNs(0) {
    mCommand.mValue = kUnallocatedCommand;
}

TimeTask::~TimeTask() {
    Stop();
}

void TimeTask::Print(std::ostream &stream) {
    stream << kDescription;
}

void TimeTask::Start(long long nEpochOffsetNs) {
    mNextNs = mClock->Now();
    if (nEpochOffsetNs == kNoEpochOffset) {
        mEpochNs = 0;
    } else {
        mEpochNs = mNextNs - nEpochOffsetNs;
    }
    Run();
}

void TimeTask::Run() {
    if (Tick(mNextNs - mEpochNs) != 1) {
        return;
    }

    mNextNs += mPeriodNs;
    Cmd *pCommand = new Cmd(this);
    const Sch::Time due{mNextNs};
    mClock->Post(pCommand, due, mCommand, kNotRecordable, kAbsolute);
    if (pCommand != nullptr) {
        pCommand->Release();
    }
}

void TimeTask::Stop() {
    const Sch::CmdID command = mCommand;
    mClock->Withdraw(command);
    mCommand.mValue = kUnallocatedCommand;
}
