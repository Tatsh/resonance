#include "app/tnlpendingtrigger.h"

#include "app/tnltrigger.h"

// 0x004572c8
int TnlPendingTrigger::Update(float flFrame) {
    if (mFrame <= flFrame) {
        mTrigger->Fire();
        delete mTrigger;
        return 0;
    }
    return 1;
}
