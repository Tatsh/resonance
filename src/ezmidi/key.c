#include "ezmidi/synth.h"

// EZMIDI 0x7c40
unsigned int gReg_kon[2];

// EZMIDI 0x7c48
unsigned int gReg_koff[2];

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
