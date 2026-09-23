#include "gs/voxer.h"

#include "game/trackdata.h"
#include "msg/erasemsg.h"
#include "msg/invalidateseekermsg.h"
#include "msg/pitchriffmsg.h"
#include "msg/stopriffmsg.h"
#include "msg/trackselectmsg.h"

// 0x001d9050
void Voxer::HandleMessage(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == static_cast<int>(g_nPitchRiffMsgType)) {
        OnPitchRiff(pMsg);
        return;
    }
    if (nType == static_cast<int>(g_nStopRiffMsgType)) {
        OnStopRiff(pMsg);
        return;
    }
    if (nType == static_cast<int>(g_nEraseMsgType)) {
        EraseMsg *pErase = static_cast<EraseMsg *>(pMsg);
        if (pErase->mUnknown0c != mUnknown44) {
            return;
        }
        if (mUnknown50 != pErase->mUnknown04) {
            return;
        }
        OnErase(pErase->mUnknown08.mTick / mUnknown48, pErase->mUnknown10, 1);
        return;
    }
    if (nType == static_cast<int>(g_dwTrackSelectMsgType)) {
        OnTrackSelect(pMsg);
        return;
    }
    if (nType == static_cast<int>(g_nInvalidateSeekerMsgType)) {
        InvalidateSeekerMsg *pInvalidate = static_cast<InvalidateSeekerMsg *>(pMsg);
        if (pInvalidate->mUnknown08 == mUnknown44) {
            OnInvalidateSeeker(pInvalidate->mUnknown04);
        }
    }
}

// 0x001d9e40
void Voxer::OnEraseMsg(EraseMsg *pMsg) {
    if (pMsg->mUnknown0c != mUnknown44) {
        return;
    }
    if (mUnknown50 != pMsg->mUnknown04) {
        return;
    }
    OnErase(pMsg->mUnknown08.mTick / mUnknown48, pMsg->mUnknown10, 1);
}

// 0x001d9e98
void Voxer::OnInvalidateSeekerMsg(InvalidateSeekerMsg *pMsg) {
    if (pMsg->mUnknown08 == mUnknown44) {
        OnInvalidateSeeker(pMsg->mUnknown04);
    }
}

// 0x001d9ec8
int Voxer::QueryBar(int nBar) {
    return mTrackData->QueryBar(nBar);
}
