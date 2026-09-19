#include "memcard/memcardps2.h"

#include <libmc.h>

#include "memcard/memcardop.h"

// `sceMcSync()` mode that reports the state of the current command without waiting for it.
constexpr int kMemcardSyncCheck = 1;

// 0x0055e268
MemcardPS2::MemcardPS2() {
}

// 0x0055e300
MemcardPS2::~MemcardPS2() {
}

// 0x0055cfc0
void MemcardPS2::Update() {
    if (mOps.empty()) {
        return;
    }

    if (mOps.front()->mIssued == kMemcardOpNotIssued) {
        mOps.front()->Issue();
        return;
    }

    if (mOps.front()->mIssued != kMemcardOpInFlight) {
        return;
    }

    int nCommand;
    int nResult;
    if (sceMcSync(kMemcardSyncCheck, &nCommand, &nResult) == 0) {
        return;
    }

    mOps.front()->mResult = nResult;
    mOps.front()->Complete();
    delete mOps.front();
    mOps.pop_front();
}
