#include "ezmidi/common.h"

#include "ezmidi/imports.h"
#include "ezmidi/synth.h"

// This file mirrors the original `midi_com.c`: common support around the engine.

// EZMIDI 0x5744
int HardSynthLoadBD(int nSpuAddr, const void *pSource, int nSize) {
    return MemCpy_IOPtoSPU(nSpuAddr, pSource, nSize);
}

// EZMIDI 0x5c54
int HardSynthUpdate(void) {
    int nIndex;

    for (nIndex = 0; nIndex < 50; ++nIndex) {
        if ((gCurrentNotes[nIndex].mFlags & 1) == 0) {
            continue;
        }
        hs_update_note_and_fx(&gCurrentNotes[nIndex]);
    }
    gUpdateMask = 0;
    return 0;
}

// EZMIDI 0x8c8
int MemCpy_IOPtoSPU(int nSpuAddr, const void *pSource, int nSize) {
    sceSdVoiceTrans(0, 0, nSpuAddr, pSource, nSize);
    while (sceSdVoiceTransStatus(0, 0) == 0) {
    }
    return 0;
}
