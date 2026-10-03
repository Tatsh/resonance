#include "gs/multimuseplayer.h"

#include "sch/cmdid.h"
#include "sch/sequencer.h"
#include "sch/tickclock.h"

// NTSC-U/C: 0x001a9fb8, PAL: 0x001afd20
// mSequencer is not initialised here. Every path that reads it runs after Start(),
// which is what makes that faithful rather than a gap.
MultiMusePlayer::MultiMusePlayer(MultiMuse *pMuse, MuseParent *pParent, Sch::TickClock *pClock)
    : MuseSynth(pClock), mMuse(pMuse), mParent(pParent), mClock(pClock), mParentToldToRetain(0),
      mRunning(0) {
    if (mMuse != nullptr) {
        ++mMuse->mRefs;
    }
}

// NTSC-U/C: 0x001aa100, PAL: 0x001afe68
MultiMusePlayer::~MultiMusePlayer() {
    Stop();
    if (mMuse != nullptr) {
        mMuse->Release();
    }
    delete mSequencer;
}

// NTSC-U/C: 0x001a9b48, PAL: 0x001af8b0
void MultiMusePlayer::Start(MsgSink *pSink) {
    AddSink(pSink);
    mRunning = 1;
    // The original spells the range as begin() and end(). Both returned a raw element pointer in
    // the template library this toolchain shipped, which is the two words the body loads.
    const TickObj<MuseMsg *> *pBegin = mMuse->mEntries.data();
    mSequencer = new Sequencer<const TickObj<MuseMsg *> *>(pBegin, pBegin + mMuse->mEntries.size());
    mSequencer->Post(mClock, this);
}

// NTSC-U/C: 0x001aa1b8, PAL: 0x001aff20
void MultiMusePlayer::Stop() {
    if (mRunning == 0) {
        return;
    }
    ReleaseAllPlayers();
    mSequencer->Withdraw();
    mRunning = 0;
}

// NTSC-U/C: 0x001a9ed0, PAL: 0x001afc38
int MultiMusePlayer::DisplacesSiblings() {
    return 1;
}

// NTSC-U/C: 0x001aa1f8, PAL: 0x001aff60
void MultiMusePlayer::RetainOnly(MusePlayer *pPlayer) {
    if (mParentToldToRetain == 0) {
        mParentToldToRetain = 1;
        mParent->RetainOnly(this);
    }
    MuseSynth::RetainOnly(pPlayer);
}

// NTSC-U/C: 0x001a9c30, PAL: 0x001af998
void MultiMusePlayer::PlayerFinished(MusePlayer *pPlayer) {
    MuseSynth::PlayerFinished(pPlayer);
    if (mSequencer->mCursor != mSequencer->mFinish) {
        return;
    }

    int nRemaining = 0;
    std::list<MusePlayer *>::iterator it = mPlayers.begin();
    for (; it != mPlayers.end(); ++it) {
        ++nRemaining;
    }
    if (nRemaining != 0) {
        return;
    }

    mRunning = 0;
    mParent->PlayerFinished(this);
}
