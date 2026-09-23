#include "msg/musemsg.h"

MuseMsg *MuseMsg::CloneAt(int nTick) {
    MuseMsg *pCopy = static_cast<MuseMsg *>(Clone());
    pCopy->mTick = nTick;
    return pCopy;
}
