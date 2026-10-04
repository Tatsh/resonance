#include "app/tnlpendingtrigger.h"

#include "app/tnltrigger.h"

int TnlPendingTrigger::Update(float flFrame) {
    if (mFrame <= flFrame) {
        mTrigger->Fire(flFrame);
        delete mTrigger;
        return 0;
    }
    return 1;
}
