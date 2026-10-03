#include "memcard/memcardps2.h"

#include <libmc.h>

#include "memcard/memcardop.h"

// `sceMcSync()` mode that reports the state of the current command without waiting for it.
constexpr int kMemcardSyncCheck = 1;

// NTSC-U/C: 0x0055e268, PAL: 0x0059f538
MemcardPS2::MemcardPS2() {
}

// NTSC-U/C: 0x0055e300, PAL: 0x0059f5d0
MemcardPS2::~MemcardPS2() {
}

// NTSC-U/C: 0x0055cfc0, PAL: 0x0059e208
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
