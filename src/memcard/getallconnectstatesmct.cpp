#include "memcard/getallconnectstatesmct.h"

#include <libmc.h>

#include "memcard/checkinfoop.h"
#include "memcard/memcard.h"
#include "memcard/memcarduser.h"

namespace {

// First multi-tap entry in the parallel slot tables, and the entry after the last one port 1 owns.
constexpr int kFirstMultiTapSlot = 2;
constexpr int kPastLastPort1MultiTapSlot = 6;

} // namespace

GetAllConnectStatesMCT::GetAllConnectStatesMCT(MemcardUser *pUser,
                                               Memcard *pCard,
                                               int nPortSlot,
                                               int nCookie)
    : MemcardTask(pUser, pCard, nPortSlot, nCookie) {
}

// 0x00184b08
GetAllConnectStatesMCT::~GetAllConnectStatesMCT() {
}

// 0x00178138
void GetAllConnectStatesMCT::OnCheckInfo(CheckInfoOp *pOp) {
    mStatus = pOp->mStatus;
    if (mStatus != kMemcardStatusUnknown) {
        int nSlot = mSlotIndex[mCompleted];
        MemcardConnectState state;
        state.mPortSlot = g_anMemcardSlotPortSlot[nSlot];
        state.mSlotName = g_apszMemcardSlotNames[nSlot];
        state.mFree = pOp->mFree;
        state.mType = pOp->mType;
        state.mFormatted = pOp->mFormatted == 1 ? 1 : 0;
        mStates->push_back(state);
    }

    ++mCompleted;
    if (mCompleted == mExpected) {
        Finish();
    }
}

// 0x001860a0
void GetAllConnectStatesMCT::Finish() {
    mState = kMemcardTaskFinished;
    mUser->OnAllConnectStates();
}

// 0x001860d8
void GetAllConnectStatesMCT::Execute() {
    mState = kMemcardTaskRunning;

    if (sceMcGetSlotMax(kMemcardPort1) != kMemcardSlotsWithoutMultiTap) {
        for (int i = kFirstMultiTapSlot; i < kPastLastPort1MultiTapSlot; ++i) {
            mCard->CheckInfo(this, g_anMemcardSlotPortSlot[i], mCookie);
            mSlotIndex[i - kFirstMultiTapSlot] = i;
        }

        mExpected = kPastLastPort1MultiTapSlot - kFirstMultiTapSlot;
        return;
    }

    mCard->CheckInfo(this, g_anMemcardSlotPortSlot[0], mCookie);
    mSlotIndex[0] = 0;
    mExpected = 1;

    // A multi-tap on the second port yields no enquiry at all, rather than the four the first port
    // would yield, which is what leaves the port 2 entries of both slot tables unreferenced.
    if (sceMcGetSlotMax(kMemcardPort2) == kMemcardSlotsWithoutMultiTap) {
        mCard->CheckInfo(this, g_anMemcardSlotPortSlot[1], mCookie);
        mSlotIndex[1] = 1;
        mExpected = 2;
    }
}
