#include "game/autoriffer.h"

#include <cstring>
#include <iostream>

#include "game/nullplayer.h"
#include "sch/command.h"

namespace {

// The handle value of a command the clock has not queued yet.
constexpr int kUnallocatedCommand = -2;

/**
 * Scheduler command that runs AutoRiffer::OnCommand() at one song position.
 *
 * `Q233_GLOBAL_$N$GsAutoRiffer.cppdKuhgb3Cmd` in the RTTI, with Sch::Command as its one base and
 * its vtable at `0x007dd3a8`. AutoRiffer::PlayRiff() expands the constructor into its 0x14-byte
 * allocation.
 *
 * The destructor at `0x0019a7c0` is implicitly declared.
 */
class Cmd : public Sch::Command {
public:
    Cmd(AutoRiffer *pOwner, int nTick) : mOwner(pOwner), mTick(nTick) {
    }

    // 0x0019a838
    virtual int CmdID() {
        return sCmdID;
    }

    // 0x0019a848
    virtual void Execute() {
        mOwner->OnCommand(mTick);
    }

    // 0x0019a868
    virtual void Print(std::ostream &stream) {
        stream << "{AutoRiffer}";
    }

    // The word at 0x006815e0, which the image initialises to zero.
    static int sCmdID;

private:
    AutoRiffer *mOwner; // +0x0c
    int mTick;          // +0x10
};

int Cmd::sCmdID;

} // namespace

// 0x00199040
AutoRiffer::AutoRiffer(Sch::TickClock *pClock, Quantizer *pQuantizer, const TrackData *pTrackData)
    : mTrack(pTrackData->mUnknown04), mQuantizer(pQuantizer), mTrackData(pTrackData),
      mCurrentRiff(nullptr), mClock(pClock), mSynth(nullptr), mPhraseMaker(nullptr),
      mPlayer(&g_nullPlayer) {
    mCommand.mValue = kUnallocatedCommand;
    std::memset(mLevelHeld, 0, sizeof(mLevelHeld));
}

// 0x0019a3d0
AutoRiffer::~AutoRiffer() {
}

// 0x0019a508
void AutoRiffer::AddSink(MsgSink *pSink) {
    mSource.AddSink(pSink);
}
