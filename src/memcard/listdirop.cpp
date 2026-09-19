#include "memcard/listdirop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

sceMcTblGetDir g_aMemcardDirEntries[kListDirMaxEntries];

ListDirOp::ListDirOp(
    MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, void *pCookie, unsigned nMode)
    : MemcardOp(pHandler, nPortSlot, pCookie), mPath(path), mMode(nMode) {
}

// 0x0055d868
ListDirOp::~ListDirOp() {
}

// 0x0055e818
void ListDirOp::Issue() {
    sceMcGetDir(mPortSlot >> kMemcardPortShift,
                mPortSlot & kMemcardSlotMask,
                mPath.mStr != nullptr ? mPath.mStr : g_szEmptyString,
                mMode,
                kListDirMaxEntries,
                g_aMemcardDirEntries);
    mIssued = kMemcardOpInFlight;
}

// 0x0055d8d0
void ListDirOp::Complete() {
    InterpretResult();
    mHandler->OnListDir(this);
}

// 0x0055e870
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
