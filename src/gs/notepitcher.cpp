#include "gs/notepitcher.h"

#include "msg/erasemsg.h"
#include "msg/eraseoffmsg.h"
#include "msg/invalidateseekermsg.h"
#include "msg/pitchriffmsg.h"
#include "msg/trackselectmsg.h"

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
