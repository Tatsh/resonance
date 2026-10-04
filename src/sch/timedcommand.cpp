#include "sch/timedcommand.h"

#include <iostream>

#include "os/hxstr.h"
#include "sch/command.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

namespace Sch {

TimedCommand::TimedCommand(Command *pCommand, Time tick, int bDelta)
    : mCommand(pCommand), mOrder(-1), mDueTick{-1}, mLocalTick(tick), mDelta(bDelta), mCmdID{-1} {
    // The store of pCommand sits in the branch delay slot of the null test and therefore runs
    // whether the test passes or not. Only the reference count is conditional.
    if (mCommand != nullptr) {
        ++mCommand->mRefs;
    }
}

TimedCommand::~TimedCommand() {
    if (mCommand != nullptr) {
        mCommand->Release();
    }
}

void TimedCommand::Run() {
    mCommand->Execute();
}

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

void TimedCommand::Save(OBStream &stream) {
    mDueTick.Save(stream); // Yes, the binary discards this call's result.
    stream.WriteLE(&mOrder, sizeof(mOrder));
    mLocalTick.Save(stream);
    stream << mDelta;
    mCmdID.Save(stream);
    stream << mCommand;
}

void TimedCommand::Load(IBStream &stream) {
    mDueTick.Load(stream); // Yes, the binary discards this call's result.
    stream.ReadLE(&mOrder, sizeof(mOrder));
    mLocalTick.Load(stream);
    stream >> mDelta;
    mCmdID.Load(stream);
    stream >> mCommand;
}

} // namespace Sch
