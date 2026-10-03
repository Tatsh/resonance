#include "gs/noteplayer.h"

#include <algorithm>
#include <iostream>

#include "mid/tick.h"
#include "msg/stdmidimsg.h"
#include "sch/command.h"

namespace {

// The handle value of a command the clock has not queued yet.
constexpr int kUnallocatedCommand = -2;

// The low four bits of a status byte select the channel.
constexpr unsigned char kChannelMask = 0xf;

// The constructor ends each note this many ticks early, and never shorter than one tick.
constexpr int kReleaseTicks = 2;
constexpr int kMinimumDuration = 1;

// The MIDI status nibbles and the note-off velocity.
constexpr unsigned char kNoteOffStatus = 0x80;
constexpr unsigned char kNoteOnStatus = 0x90;
constexpr unsigned char kReleaseVelocity = 0;

// The clamp the inline Sch::Tick arithmetic applies to a computed position.
inline int ClampPosition(int nTick) {
    return std::min(std::max(nTick, kTickMinimum), kTickMaximum);
}

/**
 * Scheduler command that runs NotePlayer::OnCommand() at the end of a note.
 *
 * `Q233_GLOBAL_$N$GsNotePlayer.cppdKuhgb3Cmd` in the RTTI, with Sch::Command as its one base and
 * its vtable at `0x007e1728`. NotePlayer::Start() expands the constructor into its 0x14-byte
 * allocation.
 *
 * The destructor at `0x001b4250` is implicitly declared.
 */
class Cmd : public Sch::Command {
public:
    Cmd(NotePlayer *pOwner, int nTick) : mOwner(pOwner), mTick(nTick) {
    }

    // NTSC-U/C: 0x001b42c8, PAL: 0x001ba0a0
    virtual int CmdID() {
        return sCmdID;
    }

    // NTSC-U/C: 0x001b42d8, PAL: 0x001ba0b0
    virtual void Execute() {
        mOwner->OnCommand(mTick);
    }

    // NTSC-U/C: 0x001b42f8, PAL: 0x001ba0d0
    virtual void Print(std::ostream &stream) {
        stream << "{MidiNoteOff}";
    }

    // The word at 0x00688668, which the image initialises to zero.
    static int sCmdID;

private:
    NotePlayer *mOwner; // +0x0c
    int mTick;          // +0x10
};

int Cmd::sCmdID;

} // namespace

// NTSC-U/C: 0x001b4328, PAL: 0x001ba100
NotePlayer::NotePlayer(unsigned char nNote,
                       unsigned char nVelocity,
                       int nDuration,
                       unsigned char nChannel,
                       MuseParent *pParent,
                       Sch::TickClock *pClock)
    : mNote(nNote), mVelocity(nVelocity), mChannel(nChannel & kChannelMask), mDuration(nDuration),
      mSink(nullptr), mParent(pParent), mClock(pClock) {
    mCommand.mValue = kUnallocatedCommand;
    mDuration = ClampPosition(mDuration - Sch::Tick(kReleaseTicks).mTick);
    if (mDuration < Sch::Tick(kMinimumDuration).mTick) {
        mDuration = Sch::Tick(kMinimumDuration).mTick;
    }
}

// NTSC-U/C: 0x001b4460, PAL: 0x001ba238
NotePlayer::~NotePlayer() {
    Stop();
}

// NTSC-U/C: 0x001b3d58, PAL: 0x001b9b30
void NotePlayer::Start(MsgSink *pSink) {
    mSink = pSink;
    const int nNow = mClock->SongTick();
    mParent->RetainOnly(this);
    NoteOn(nNow);

    Cmd *pCommand = new Cmd(this, Sch::Tick(ClampPosition(nNow + mDuration)).mTick);
    const Sch::Tick end(ClampPosition(nNow + mDuration));
    mClock->PostAtSongTick(pCommand, end.mTick, mCommand);
    if (pCommand != nullptr) {
        pCommand->Release();
    }
}

// NTSC-U/C: 0x001b3ee0, PAL: 0x001b9cb8
void NotePlayer::Stop() {
    if (mSink != nullptr) {
        StdMidiMsg msg(mClock->SongTick(), kNoteOffStatus | mChannel, mNote, kReleaseVelocity);
        mSink->Dispatch(&msg);
        mClock->Withdraw(mCommand);
    }
    mSink = nullptr;
}

// NTSC-U/C: 0x001b3fb8, PAL: 0x001b9d90
void NotePlayer::NoteOn(int nTick) {
    StdMidiMsg msg(nTick, kNoteOnStatus | mChannel, mNote, mVelocity);
    mSink->Dispatch(&msg);
}

// NTSC-U/C: 0x001b4040, PAL: 0x001b9e18
void NotePlayer::OnCommand(int nTick) {
    StdMidiMsg msg(nTick, kNoteOffStatus | mChannel, mNote, kReleaseVelocity);
    mSink->Dispatch(&msg);
    mSink = nullptr;
    mParent->PlayerFinished(this);
}

// NTSC-U/C: 0x001b41c0, PAL: 0x001b9f98
int NotePlayer::DisplacesSiblings() {
    return 0;
}
