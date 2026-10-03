#include "memcard/getconnectstatemct.h"

#include <libmc.h>
#include <libmtap.h>

#include "memcard/checkinfoop.h"
#include "memcard/memcard.h"
#include "memcard/memcarduser.h"

namespace {

// First multi-tap entry in the parallel slot tables.
constexpr int kFirstMultiTapSlot = 2;

} // namespace

// NTSC-U/C: 0x001848f8, PAL: 0x00189e00
GetConnectStateMCT::GetConnectStateMCT(MemcardUser *pUser,
                                       Memcard *pCard,
                                       int nPortSlot,
                                       int nCookie)
    : MemcardTask(pUser, pCard, nPortSlot, nCookie) {
}

// NTSC-U/C: 0x001849c8, PAL: 0x00189ed0
GetConnectStateMCT::~GetConnectStateMCT() {
}

// NTSC-U/C: 0x00177fb0, PAL: 0x0017b370
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
    if (sceMtapGetConnection(kMemcardPort1) == kMemcardMultitapConnected) {
        for (int i = kFirstMultiTapSlot; i < kMemcardSlotCount; ++i) {
            if (g_anMemcardSlotPortSlot[i] == mPortSlot) {
                pszSlotName = g_apszMemcardSlotNames[i];
                break;
            }
        }
    } else if (g_anMemcardSlotPortSlot[0] == mPortSlot) {
        pszSlotName = g_apszMemcardSlotNames[0];
    } else if (sceMtapGetConnection(kMemcardPort2) == kMemcardMultitapConnected) {
        // The four multi-tap entries of port 2 are never searched, so a slot behind a tap on the
        // second port cannot be identified at all.
        mStatus = kMemcardStatusUnknown;
    } else {
        pszSlotName = g_apszMemcardSlotNames[1];
    }

    mConnectState.mSlotName = pszSlotName;
    mConnectState.mType = pOp->mType;
    mConnectState.mFormatted = pOp->mFormatted == 1 ? 1 : 0;
#ifdef VIDEO_STANDARD_PAL
    mConnectState.mFree = MemcardConnectStateFree(pOp->mFormatted, pOp->mFree);
#else
    mConnectState.mFree = pOp->mFree;
#endif
    Finish();
}

// NTSC-U/C: 0x00185ff0, PAL: 0x0018baa8
void GetConnectStateMCT::Finish() {
    mState = kMemcardTaskFinished;
    mUser->OnConnectState(mConnectState, mStatus);
}

// NTSC-U/C: 0x00186070, PAL: 0x0018bb28
void GetConnectStateMCT::Execute() {
    mState = kMemcardTaskRunning;
    mCard->CheckInfo(this, mPortSlot, mCookie);
}

const int g_anMemcardSlotPortSlot[kMemcardSlotCount] = {
    0,
    0x100,
    0,
    1,
    2,
    3,
    0x100,
    0x101,
    0x102,
    0x103,
};

const char *const g_apszMemcardSlotNames[kMemcardSlotCount] = {
    "1",
    "2",
    "1-A",
    "1-B",
    "1-C",
    "1-D",
    "2-A",
    "2-B",
    "2-C",
    "2-D",
};
