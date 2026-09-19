#include "sch/timedcommand.h"

#include <iostream>

#include "os/hxstr.h"
#include "sch/command.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

namespace Sch {

// 0x005d32f8
TimedCommand::TimedCommand(Command *pCommand, Tick tick, int bDelta)
    : mCommand(pCommand), mOrder(-1), mDueTick{-1}, mLocalTick(tick), mDelta(bDelta), mCmdID{-1} {
    // The store of pCommand sits in the branch delay slot of the null test and therefore runs
    // whether the test passes or not. Only the reference count is conditional.
    if (mCommand != nullptr) {
        ++mCommand->mRefs;
    }
}

// 0x005d33e8
TimedCommand::~TimedCommand() {
    if (mCommand != nullptr) {
        mCommand->Release();
    }
}

// 0x005d33b8
void TimedCommand::Run() {
    mCommand->Execute();
}

// 0x005d30b0
void TimedCommand::Print(std::ostream &stream) {
    HxStr sMode(" abs");
    if (mDelta != 0) {
        sMode = " delta";
    }
    stream << '[';
    mDueTick.Print(stream);
    stream << " local:";
    mLocalTick.Print(stream);
    stream << sMode << " id:";
    mCmdID.Print(stream);
    stream << " ";
    mCommand->Print(stream);
    stream << ']';
}

// 0x005d3440
void TimedCommand::Save(OBStream &stream) {
    mDueTick.Save(stream); // Yes, the binary discards this call's result.
    stream.Write(&mOrder, sizeof(mOrder));
    mLocalTick.Save(stream);
    stream << mDelta;
    mCmdID.Save(stream);
    stream << mCommand;
}

// 0x005d34d0
void TimedCommand::Load(IBStream &stream) {
    mDueTick.Load(stream); // Yes, the binary discards this call's result.
    stream.Read(&mOrder, sizeof(mOrder));
    mLocalTick.Load(stream);
    stream >> mDelta;
    mCmdID.Load(stream);
    stream >> mCommand;
}

} // namespace Sch
