#include "msg/musemsg.h"

// 0x003e3620
MuseMsg *MuseMsg::CloneAt(int nTick) {
    MuseMsg *pCopy = static_cast<MuseMsg *>(Clone());
    pCopy->mTick = nTick;
    return pCopy;
}
