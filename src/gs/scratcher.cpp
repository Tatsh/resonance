#include "gs/scratcher.h"

#include "game/idablebase.h"
#include "game/nullplayer.h"
#include "game/trackdata.h"
#include "gs/phrasemgr.h"
#include "msg/axisregistermsg.h"
#include "msg/erasemsg.h"
#include "msg/invalidateseekermsg.h"
#include "msg/pitchriffmsg.h"
#include "msg/trackselectmsg.h"
#include "script/configquery.h"

namespace {

// The value the constructor gives mUnknown6c.
constexpr int kNoValue = -1;

// mUnknown74 starts with this many elements, each built from the int zero.
constexpr int kUnknown74Count = 3;
constexpr int kUnknown74Initial = 0;

// The two configuration codes that decide mUnknown54.
constexpr int kBankSwitchConfigCode = 0x3a4;
constexpr int kBankSwitchOverrideConfigCode = 0x3a1;

} // namespace

// 0x001cf988
Scratcher::Scratcher(PhraseMgr *pPhraseMgr,
                     Quantizer *pQuantizer,
                     Sch::TickClock *pClock,
                     const TrackData *pTrackData)
    : Pitcher(pClock), mPhraseMgr(pPhraseMgr), mQuantizer(pQuantizer), mTrackData(pTrackData),
      mUnknown44(pTrackData->mUnknown04), mBarDivisor(pPhraseMgr->mBarTicks), mClock(pClock),
      mUnknown50(kIDableUnregistered), mUnknown5c(&g_nullPlayer), mUnknown64(&g_nullPlayer),
      mUnknown68(0), mUnknown6c(kNoValue), mUnknown70(0),
      mUnknown74(kUnknown74Count, kUnknown74Initial), mUnknown84(0), mUnknown88(0), mUnknown8c(0) {
    mUnknown54 = 0;
    if (QueryConfigFlag(kBankSwitchConfigCode) != 0) {
        mUnknown54 = QueryConfigFlag(kBankSwitchOverrideConfigCode) == 0;
    }
    mUnknown58 = pTrackData->mChannel;
}

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

// 0x001d1cc8
void Scratcher::OnPitchRiffMsg(PitchRiffMsg *pMsg) {
    if (pMsg->mUnknown10 != mUnknown44) {
        return;
    }
    if (mUnknown5c != pMsg->mUnknown08) {
        return;
    }
    mUnknown68 = OnPitchRiff(pMsg->mUnknown04, 0, pMsg->mUnknown0c.mTick);
}

// 0x001d1d18
void Scratcher::OnInvalidateSeeker(InvalidateSeekerMsg *pMsg) {
    if (pMsg->mUnknown08 == mUnknown44) {
        SendSeekerMsg(pMsg->mUnknown04);
    }
}

// 0x001d1d48
int Scratcher::QueryBar(int nBar) {
    return mTrackData->QueryBar(nBar);
}
