#include "game/phraseeraser.h"

#include "gs/phrasemgr.h"

// NTSC-U/C: 0x001b99c8, PAL: 0x001bf7a0
PhraseEraser::PhraseEraser(
    int nTrack, int nSecondArgument, PhraseMgr *pPhraseMgr, int nFourthArgument, int nFifthArgument)
    : mActive(0), mFifthArgument(nFifthArgument), mTrack(nTrack), mPhraseMgr(pPhraseMgr),
      mSecondArgument(nSecondArgument), mFourthArgument(nFourthArgument) {
}

// NTSC-U/C: 0x001b9a60, PAL: 0x001bf838
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

// NTSC-U/C: 0x001b9ac8, PAL: 0x001bf8a0
void PhraseEraser::EraseBar(int) {
}

// NTSC-U/C: 0x001b9ad0, PAL: 0x001bf8a8
void PhraseEraser::HandleMessage(Message *pMsg) {
    if (pMsg->Type() != g_nEraseMsgType) {
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
