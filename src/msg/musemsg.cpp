#include "msg/musemsg.h"

MuseMsg *MuseMsg::CloneAndShift(int nTick) {
    MuseMsg *pCopy = static_cast<MuseMsg *>(Clone());
    pCopy->mTick = nTick;
    return pCopy;
}
