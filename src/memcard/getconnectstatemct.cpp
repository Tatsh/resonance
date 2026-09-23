#include "memcard/getconnectstatemct.h"

#include <libmc.h>

#include "memcard/checkinfoop.h"
#include "memcard/memcard.h"
#include "memcard/memcarduser.h"

namespace {

// First multi-tap entry in the parallel slot tables.
constexpr int kFirstMultiTapSlot = 2;

} // namespace

// 0x001848f8
GetConnectStateMCT::GetConnectStateMCT(MemcardUser *pUser,
                                       Memcard *pCard,
                                       int nPortSlot,
                                       int nCookie)
    : MemcardTask(pUser, pCard, nPortSlot, nCookie) {
}

// 0x001849c8
GetConnectStateMCT::~GetConnectStateMCT() {
}

// 0x00177fb0
void GetConnectStateMCT::OnCheckInfo(CheckInfoOp *pOp) {
    mStatus = pOp->mStatus;
    mConnectState.mPortSlot = mPortSlot;
    if (mStatus == kMemcardStatusUnknown) {
        // Every handler in this family shares the shape, and on this branch the test cannot
        // succeed, because the status is already known to be kMemcardStatusUnknown.
        if (mStatus != kMemcardStatusOk) {
            mCard->Cancel(mCookie);
            Finish();
        }
        return;
    }

    const char *pszSlotName = "";
    if (sceMcGetSlotMax(kMemcardPort1) != kMemcardSlotsWithoutMultiTap) {
        for (int i = kFirstMultiTapSlot; i < kMemcardSlotCount; ++i) {
            if (g_anMemcardSlotPortSlot[i] == mPortSlot) {
                pszSlotName = g_apszMemcardSlotNames[i];
                break;
            }
        }
    } else if (g_anMemcardSlotPortSlot[0] == mPortSlot) {
        pszSlotName = g_apszMemcardSlotNames[0];
    } else if (sceMcGetSlotMax(kMemcardPort2) != kMemcardSlotsWithoutMultiTap) {
        // The four multi-tap entries of port 2 are never searched, so a slot behind a tap on the
        // second port cannot be identified at all.
        mStatus = kMemcardStatusUnknown;
    } else {
        pszSlotName = g_apszMemcardSlotNames[1];
    }

    mConnectState.mSlotName = pszSlotName;
    mConnectState.mType = pOp->mType;
    mConnectState.mFormatted = pOp->mFormatted == 1 ? 1 : 0;
    mConnectState.mFree = pOp->mFree;
    Finish();
}

// 0x00185ff0
void GetConnectStateMCT::Finish() {
    mState = kMemcardTaskFinished;
    mUser->OnConnectState(mConnectState, mStatus);
}

// 0x00186070
void GetConnectStateMCT::Execute() {
    mState = kMemcardTaskRunning;
    mCard->CheckInfo(this, mPortSlot, mCookie);
}
