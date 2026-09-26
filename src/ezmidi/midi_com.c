#include "ezmidi/common.h"

#include "ezmidi/imports.h"

// This file mirrors the original `midi_com.c`: common support around the engine.

// EZMIDI 0x5744
int HardSynthLoadBD(int nSpuAddr, const void *pSource, int nSize) {
    return MemCpy_IOPtoSPU(nSpuAddr, pSource, nSize);
}

// EZMIDI 0x8c8
int MemCpy_IOPtoSPU(int nSpuAddr, const void *pSource, int nSize) {
    sceSdVoiceTrans(0, 0, nSpuAddr, pSource, nSize);
    while (sceSdVoiceTransStatus(0, 0) == 0) {
    }
    return 0;
}
