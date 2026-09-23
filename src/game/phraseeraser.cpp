#include "game/phraseeraser.h"

#include "gs/phrasemgr.h"

// 0x001b99c8
PhraseEraser::PhraseEraser(
    int nTrack, int nUnknown34, PhraseMgr *pPhraseMgr, int nUnknown38, int nUnknown28)
    : mActive(0), mUnknown28(nUnknown28), mTrack(nTrack), mPhraseMgr(pPhraseMgr),
      mUnknown34(nUnknown34), mUnknown38(nUnknown38) {
}

// 0x001b9a60
void PhraseEraser::OnEraseMsg(EraseMsg *pMsg) {
    if (pMsg->mUnknown0c != mTrack) {
        return;
    }
    mActive = 1;
    const int nBar = pMsg->mUnknown08.mTick / mPhraseMgr->mBarTicks;
    mFirstBar = nBar;
    mLastBar = nBar;
    mPlayer = pMsg->mUnknown04;
    EraseBar(nBar);
}

// 0x001b9ac8
void PhraseEraser::EraseBar(int) {
}

// 0x001b9ad0
void PhraseEraser::HandleMessage(Message *pMsg) {
    if (pMsg->Type() != g_nEraseMsgType) {
        return;
    }
    EraseMsg *pErase = static_cast<EraseMsg *>(pMsg);
    if (pErase->mUnknown0c != mTrack) {
        return;
    }
    mActive = 1;
    const int nBar = pErase->mUnknown08.mTick / mPhraseMgr->mBarTicks;
    mFirstBar = nBar;
    mLastBar = nBar;
    mPlayer = pErase->mUnknown04;
    EraseBar(nBar);
}
