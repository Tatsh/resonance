#include "app/tnlpendingtrigger.h"

#include "app/tnltrigger.h"

// NTSC-U/C: 0x004572c8, PAL: 0x004947f8
int TnlPendingTrigger::Update(float flFrame) {
    if (mFrame <= flFrame) {
        mTrigger->Fire(flFrame);
        delete mTrigger;
        return 0;
    }
    return 1;
}
