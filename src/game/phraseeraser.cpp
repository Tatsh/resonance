#include "game/phraseeraser.h"

#include "gs/phrasemgr.h"

PhraseEraser::PhraseEraser(
    int nTrack, int nSecondArgument, PhraseMgr *pPhraseMgr, int nFourthArgument, int nFifthArgument)
    : mActive(0), mFifthArgument(nFifthArgument), mTrack(nTrack), mPhraseMgr(pPhraseMgr),
      mSecondArgument(nSecondArgument), mFourthArgument(nFourthArgument) {
}

void PhraseEraser::OnEraseMsg(EraseMsg *pMsg) {
    if (pMsg->mTrack != mTrack) {
        return;
    }
    mActive = 1;
    const int nBar = pMsg->mPosition.mTick / mPhraseMgr->mBarTicks;
    mFirstBar = nBar;
    mLastBar = nBar;
    mPlayer = pMsg->mPlayer;
    EraseBar(nBar);
}

void PhraseEraser::EraseBar(int) {
}

void PhraseEraser::DispatchPriv(Message *pMsg) {
    if (pMsg->Type() != EraseMsg::sID) {
        return;
    }
    EraseMsg *pErase = static_cast<EraseMsg *>(pMsg);
    if (pErase->mTrack != mTrack) {
        return;
    }
    mActive = 1;
    const int nBar = pErase->mPosition.mTick / mPhraseMgr->mBarTicks;
    mFirstBar = nBar;
    mLastBar = nBar;
    mPlayer = pErase->mPlayer;
    EraseBar(nBar);
}
