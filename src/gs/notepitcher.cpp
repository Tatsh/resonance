#include "gs/notepitcher.h"

#include "game/nullplayer.h"
#include "game/trackdata.h"
#include "gs/phrasemgr.h"
#include "msg/erasemsg.h"
#include "msg/eraseoffmsg.h"
#include "msg/invalidateseekermsg.h"
#include "msg/pitchriffmsg.h"
#include "msg/trackselectmsg.h"

namespace {

// The value the constructor gives mUnknown48 and mUnknown54.
constexpr int kNoValue = -1;

// The value the constructor gives mUnknown5c.
constexpr int kUnknown5cInitial = 2;

} // namespace

// 0x001b1ce0
NotePitcher::NotePitcher(PhraseMgr *pPhraseMgr,
                         Quantizer *pQuantizer,
                         Sch::TickClock *pClock,
                         const TrackData *pTrackData,
                         int bPlayModeOne,
                         int nUnknown1,
                         int nUnknown2)
    : Pitcher(pClock), mPhraseMgr(pPhraseMgr), mQuantizer(pQuantizer),
      mUnknown40(pTrackData->mUnknown04), mUnknown44(&g_nullPlayer), mUnknown48(kNoValue),
      mBarDivisor(kMBTInfinity), mUnknown54(kNoValue), mPlayModeOne(bPlayModeOne),
      mUnknown5c(kUnknown5cInitial), mUnknown60(nUnknown1), mUnknown64(nUnknown2),
      mTrackData(pTrackData), mClock(pClock) {
    // The divisor is stored twice, the placeholder and then the bar length.
    mBarDivisor = mPhraseMgr->mBarTicks;
}

// 0x001b39c0
NotePitcher::~NotePitcher() {
}

// 0x001b3b98
int NotePitcher::Tick(int nElapsedTicks) {
    PostSeekerMsgSecond(nElapsedTicks / mBarDivisor, 0);
    return 1;
}

// 0x001b3bd0
void NotePitcher::HandleMessage(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == static_cast<int>(g_nPitchRiffMsgType)) {
        PostPitchMsg(pMsg);
        return;
    }
    if (nType == static_cast<int>(g_nEraseMsgType)) {
        PostAllNotesOffMsg(pMsg);
        return;
    }
    if (nType == static_cast<int>(g_nEraseOffMsgType)) {
        return;
    }
    if (nType == static_cast<int>(g_dwTrackSelectMsgType)) {
        PostSeekerMsg(pMsg);
        return;
    }
    if (nType == static_cast<int>(g_nInvalidateSeekerMsgType)) {
        InvalidateSeekerMsg *pInvalidate = static_cast<InvalidateSeekerMsg *>(pMsg);
        if (pInvalidate->mUnknown08 == mUnknown40) {
            PostSeekerMsgSecond(pInvalidate->mUnknown04, 0);
        }
    }
}

// 0x001b3a38
void NotePitcher::OnInvalidateSeeker(InvalidateSeekerMsg *pMsg) {
    if (pMsg->mUnknown08 == mUnknown40) {
        PostSeekerMsgSecond(pMsg->mUnknown04, 0);
    }
}

// 0x001b3b88
int NotePitcher::IsOtherTick(int nTick) {
    return mUnknown4c.mTick != nTick;
}
