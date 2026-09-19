#include "sch/timedcommand.h"

namespace Sch {

// 0x005d32f8
TimedCommand::TimedCommand(Command *pCommand, Tick tick, int nUnknown20)
    : mCommand(pCommand), mUnknown0c(-1), mDueTick{-1}, mUnknown18(tick), mUnknown20(nUnknown20),
      mUnknown24(-1) {
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

} // namespace Sch
