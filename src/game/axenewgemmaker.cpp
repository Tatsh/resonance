#include "game/axenewgemmaker.h"

#include "game/nullplayer.h"
#include "msg/axisfxmsg.h"
#include "msg/axisregistermsg.h"
#include "msg/stdmidimsg.h"
#include "msg/trackselectmsg.h"

namespace {

// The axis value the constructor assumes, the middle of the range.
constexpr float kAxisCenter = 0.5f;

} // namespace

// 0x001a2da0
AxeNewGemMaker::AxeNewGemMaker(const TrackData *pTrackData)
    : mTrack(pTrackData->mUnknown04), mTrackData(pTrackData), mStripId(0), mValue(kAxisCenter),
      mPlayer(&g_nullPlayer) {
}

// 0x001a46e0
void AxeNewGemMaker::HandleMessage(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == static_cast<int>(g_dwTrackSelectMsgType)) {
        TrackSelectMsg *pSelect = static_cast<TrackSelectMsg *>(pMsg);
        if (pSelect->mUnknown04 == mTrack && pSelect->mUnknown08 == 0) {
            mPlayer = pSelect->mUnknown10;
        }
    } else if (nType == g_nAxisRegisterMsgType) {
        AxisRegisterMsg *pAxis = static_cast<AxisRegisterMsg *>(pMsg);
        if (pAxis->mPlayer == mPlayer) {
            mValue = pAxis->mValue;
        }
    } else if (nType == g_nAxisFXMsgType) {
        return;
    } else if (nType == static_cast<int>(g_dwStdMidiMsgType)) {
        PostGemMessages(static_cast<StdMidiMsg *>(pMsg));
    }
}
