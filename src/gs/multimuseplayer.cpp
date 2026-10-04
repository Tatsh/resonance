#include "gs/multimuseplayer.h"

#include "sch/cmdid.h"
#include "sch/sequencer.h"
#include "sch/tickclock.h"

MultiMusePlayer::MultiMusePlayer(MultiMuse *pMuse, MuseParent *pParent, Sch::TickClock *pClock)
    : MuseSynth(pClock), mMuse(pMuse), mParent(pParent), mClock(pClock), mParentToldToRetain(0),
      mRunning(0) {
    // mSequencer is not initialised here. Every path that reads it runs after Start().
    if (mMuse != nullptr) {
        ++mMuse->mRefs;
    }
}

MultiMusePlayer::~MultiMusePlayer() {
    Stop();
    if (mMuse != nullptr) {
        mMuse->Release();
    }
    delete mSequencer;
}

void MultiMusePlayer::Start(MsgSink *pSink) {
    AddSink(pSink);
    mRunning = 1;
    // The original spells the range as begin() and end(). Both returned a raw element pointer in
    // the template library this toolchain shipped, which is the two words the body loads.
    const TickObj<MuseMsg *> *pBegin = mMuse->mEntries.data();
    mSequencer = new Sequencer<const TickObj<MuseMsg *> *>(pBegin, pBegin + mMuse->mEntries.size());
    mSequencer->Post(mClock, this);
}

void MultiMusePlayer::Stop() {
    if (mRunning == 0) {
        return;
    }
    ReleaseAllPlayers();
    mSequencer->Withdraw();
    mRunning = 0;
}

int MultiMusePlayer::DisplacesSiblings() {
    return 1;
}

void MultiMusePlayer::RetainOnly(MusePlayer *pPlayer) {
    if (mParentToldToRetain == 0) {
        mParentToldToRetain = 1;
        mParent->RetainOnly(this);
    }
    MuseSynth::RetainOnly(pPlayer);
}

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
