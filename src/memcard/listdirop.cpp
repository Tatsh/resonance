#include "memcard/listdirop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

// The IOP writes the entries here by DMA.
// NTSC-U/C: 0x00726a40, PAL: 0x0076a700
alignas(64) sceMcTblGetDir g_aMemcardDirEntries[kListDirMaxEntries];

// NTSC-U/C: 0x0055e778, PAL: 0x0059fa48
ListDirOp::ListDirOp(
    MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, int nCookie, unsigned nMode)
    : MemcardOp(pHandler, nPortSlot, nCookie), mPath(path), mMode(nMode) {
}

// NTSC-U/C: 0x0055d868, PAL: 0x0059ead0
ListDirOp::~ListDirOp() {
}

// NTSC-U/C: 0x0055e818, PAL: 0x0059fae8
void ListDirOp::Issue() {
    sceMcGetDir(mPortSlot >> kMemcardPortShift,
                mPortSlot & kMemcardSlotMask,
                mPath.mStr != nullptr ? mPath.mStr : g_szEmptyString,
                mMode,
                kListDirMaxEntries,
                g_aMemcardDirEntries);
    mIssued = kMemcardOpInFlight;
}

// NTSC-U/C: 0x0055d8d0, PAL: 0x0059eb48
void ListDirOp::Complete() {
    InterpretResult();
    mHandler->OnListDir(this);
}

// NTSC-U/C: 0x0055e870, PAL: 0x0059fb40
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
