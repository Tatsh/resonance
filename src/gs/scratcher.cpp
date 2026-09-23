#include "gs/scratcher.h"

#include "msg/axisregistermsg.h"
#include "msg/erasemsg.h"
#include "msg/invalidateseekermsg.h"
#include "msg/pitchriffmsg.h"
#include "msg/trackselectmsg.h"

// 0x001d1bf0
Scratcher::~Scratcher() {
}

// 0x001d0980
void Scratcher::HandleMessage(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == static_cast<int>(g_nPitchRiffMsgType)) {
        PitchRiffMsg *pRiff = static_cast<PitchRiffMsg *>(pMsg);
        if (pRiff->mUnknown10 != mUnknown44) {
            return;
        }
        if (mUnknown5c != pRiff->mUnknown08) {
            return;
        }
        mUnknown68 = OnPitchRiff(pRiff->mUnknown04, 0, pRiff->mUnknown0c.mTick);
        return;
    }
    if (nType == static_cast<int>(g_nEraseMsgType)) {
        EraseGemRange(pMsg);
        return;
    }
    if (nType == static_cast<int>(g_dwTrackSelectMsgType)) {
        OnTrackSelect(pMsg);
        return;
    }
    if (nType == static_cast<int>(g_nInvalidateSeekerMsgType)) {
        InvalidateSeekerMsg *pInvalidate = static_cast<InvalidateSeekerMsg *>(pMsg);
        if (pInvalidate->mUnknown08 == mUnknown44) {
            SendSeekerMsg(pInvalidate->mUnknown04);
        }
        return;
    }
    if (nType == static_cast<int>(g_nAxisRegisterMsgType)) {
        PostNowBarMsg(pMsg);
    }
}
