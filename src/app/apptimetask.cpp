#include <iostream>

#include "app/timeclock.h"
#include "app/timetask.h"
#include "sch/command.h"
#include "sch/tick.h"

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

// NTSC-U/C: 0x0013b100, PAL: 0x0013ba48
TimeTask::TimeTask(Sch::TimeClock *pClock, long long nPeriodNs)
    : mClock(pClock), mPeriodNs(nPeriodNs), mNextNs(0), mEpochNs(0) {
    mCommand.mValue = kUnallocatedCommand;
}

// NTSC-U/C: 0x0013b138, PAL: 0x0013ba80
TimeTask::~TimeTask() {
    Stop();
}

// NTSC-U/C: 0x0013afc0, PAL: 0x0013b908
void TimeTask::Print(std::ostream &stream) {
    stream << kDescription;
}

// NTSC-U/C: 0x0013b180, PAL: 0x0013bac8
void TimeTask::Start(long long nEpochOffsetNs) {
    mNextNs = mClock->Now();
    if (nEpochOffsetNs == kNoEpochOffset) {
        mEpochNs = 0;
    } else {
        mEpochNs = mNextNs - nEpochOffsetNs;
    }
    Run();
}

// NTSC-U/C: 0x0013ae70, PAL: 0x0013b7b8
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

// NTSC-U/C: 0x0013b1f0, PAL: 0x0013bb38
void TimeTask::Stop() {
    const Sch::CmdID command = mCommand;
    mClock->Withdraw(command);
    mCommand.mValue = kUnallocatedCommand;
}
