#include "ezmidi/synth.h"

#include "ezmidi/imports.h"

// EZMIDI 0x7c40
unsigned int gReg_kon[2];

// EZMIDI 0x7c48
unsigned int gReg_koff[2];

// EZMIDI 0x6ff8
unsigned int gPrev_VMixEL[2];

// EZMIDI 0x1824
void do_kOff(int nGroup) {
    unsigned int nMask = (unsigned int)slot_2_mask.mWords[nGroup & 1];

    gReg_koff[nGroup & 1] |= nMask;
    if ((gReg_kon[nGroup & 1] & nMask) != 0) {
        gReg_kon[nGroup & 1] &= ~nMask;
    }
}

// EZMIDI 0x1964
void do_kOn(int nGroup) {
    unsigned int nMask = (unsigned int)slot_2_mask.mWords[nGroup & 1];

    gReg_kon[nGroup & 1] |= nMask;
    if ((gReg_koff[nGroup & 1] & nMask) != 0) {
        gReg_koff[nGroup & 1] &= ~nMask;
    }
}

// EZMIDI 0x4840
void _do_reg_out(void) {
    int nGroup;

    for (nGroup = 0; nGroup < 2; ++nGroup) {
        unsigned int nBoth = gReg_kon[nGroup] & gReg_koff[nGroup];

        if (nBoth != 0) {
            gReg_koff[nGroup] &= ~nBoth;
        }
        if (gReg_kon[nGroup] != 0) {
            sceSdSetSwitch((nGroup | 0x1500) & 0xffff, gReg_kon[nGroup]);
        }
        if (gReg_koff[nGroup] != 0) {
            sceSdSetSwitch((nGroup | 0x1600) & 0xffff, gReg_koff[nGroup]);
        }
        if (voice_alloc[14 + nGroup] != voice_alloc[8 + nGroup]) {
            sceSdSetSwitch((nGroup | 0x1800) & 0xffff, voice_alloc[14 + nGroup]);
        }
        if (gReg_VMixEL[nGroup] != gPrev_VMixEL[nGroup]) {
            sceSdSetSwitch((nGroup | 0x1900) & 0xffff, gReg_VMixEL[nGroup]);
        }
        if (gReg_VMixR[nGroup] != voice_alloc[2 + nGroup]) {
            sceSdSetSwitch((nGroup | 0x1a00) & 0xffff, gReg_VMixR[nGroup]);
        }
        if (gReg_VMixER[nGroup] != voice_alloc[6 + nGroup]) {
            sceSdSetSwitch((nGroup | 0x1b00) & 0xffff, gReg_VMixER[nGroup]);
        }
    }
}
