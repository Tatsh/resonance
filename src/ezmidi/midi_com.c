#include "ezmidi/common.h"

#include "ezmidi/imports.h"
#include "ezmidi/synth.h"

// This file mirrors the original `midi_com.c`: common support around the engine.

// EZMIDI 0x6e84
int gTickTrace;

// EZMIDI 0x6e50
const char gTickPre[] = "]";

// EZMIDI 0x6e54
const char gTickPost[] = "[";

// EZMIDI 0x5744
int HardSynthLoadBD(int nSpuAddr, const void *pSource, int nSize) {
    return MemCpy_IOPtoSPU(nSpuAddr, pSource, nSize);
}

// EZMIDI 0x5b7c
int HardSynthKillOld(void) {
    int nIndex;

    for (nIndex = 0; nIndex < 50; ++nIndex) {
        if ((gCurrentNotes[nIndex].mFlags & 1) == 0) {
            continue;
        }
        hs_check_playing(&gCurrentNotes[nIndex]);
    }
    return 0;
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

// EZMIDI 0x5f00
void hsyn_atick(void) {
    int nScanned;

    for (;;) {
        if (gTickTrace != 0) {
            printf(gTickPre);
        }
        SleepThread();
        if (gTickTrace != 0) {
            printf(gTickPost);
        }
        hs_tick_setup();
        HardSynthKillOld();
        nScanned = scan_inbuf(0);
        nScanned += scan_inbuf(1);
        // Yes, the binary tallies the scanned messages and never reads the tally.
        (void)nScanned;
        HardSynthUpdate();
        _do_reg_out();
    }
}

// EZMIDI 0x8c8
int MemCpy_IOPtoSPU(int nSpuAddr, const void *pSource, int nSize) {
    sceSdVoiceTrans(0, 0, nSpuAddr, pSource, nSize);
    while (sceSdVoiceTransStatus(0, 0) == 0) {
    }
    return 0;
}
