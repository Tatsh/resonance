#include "msg/musemsg.h"

// NTSC-U/C: 0x003e3620, PAL: 0x0041b9c0
MuseMsg *MuseMsg::CloneAndShift(int nTick) {
    MuseMsg *pCopy = static_cast<MuseMsg *>(Clone());
    pCopy->mTick = nTick;
    return pCopy;
}
