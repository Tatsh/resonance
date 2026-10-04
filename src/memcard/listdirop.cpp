#include "memcard/listdirop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

// The IOP writes the entries here by DMA.
// NTSC-U/C: 0x00726a40, PAL: 0x0076a700
alignas(64) sceMcTblGetDir g_aMemcardDirEntries[kListDirMaxEntries];

ListDirOp::ListDirOp(
    MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, int nCookie, unsigned nMode)
    : MemcardOp(pHandler, nPortSlot, nCookie), mPath(path), mMode(nMode) {
}

ListDirOp::~ListDirOp() {
}

void ListDirOp::Execute() {
    sceMcGetDir(mPortSlot >> kMemcardPortShift,
                mPortSlot & kMemcardSlotMask,
                mPath.mStr != nullptr ? mPath.mStr : g_szEmptyString,
                mMode,
                kListDirMaxEntries,
                g_aMemcardDirEntries);
    mIssued = kMemcardOpInFlight;
}

void ListDirOp::NotifyDone() {
    InterpretResult();
    mHandler->OnListDir(this);
}

void ListDirOp::InterpretResult() {
    if (mResult >= sceMcResSucceed) {
        mStatus = kMemcardStatusOk;
        mEntryCount = mResult;
        mTruncated = mResult == kListDirMaxEntries ? 1 : 0;
        mEntries = g_aMemcardDirEntries;
        return;
    }

    switch (mResult) {
    case sceMcResNoFormat:
        mStatus = kMemcardStatusNotFormatted;
        break;
    case sceMcResNoEntry:
        mStatus = kMemcardStatusNoDirectory;
        break;
    default:
        mStatus = kMemcardStatusUnknown;
        break;
    }
}
