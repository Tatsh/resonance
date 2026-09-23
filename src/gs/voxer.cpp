#include "gs/voxer.h"

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
        // The cast records a disagreement rather than a conversion. EraseMsg types its `+0x04`
        // as an int while this class stores the same object pointer there, and the same value
        // reaches Mixer::mSelection from a TrackSelectMsg.
        if (mUnknown50 != reinterpret_cast<Player *>(pErase->mUnknown04)) {
            return;
        }
        OnErase(pErase->mUnknown08 / mUnknown48, pErase->mUnknown10, 1);
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
