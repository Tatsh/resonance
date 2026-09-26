#include "ezmidi/synth.h"

#include "ezmidi/imports.h"

// EZMIDI image-relative addresses appear in the markers below. The module is an IOP
// program of its own, so they never coincide with the main program. This file mirrors
// the original `midi_hsyn.c`: the synthesiser voice engine.

// EZMIDI 0x8e00
struct MidiChannel gChan[16];

// EZMIDI 0x8f00
struct BankData *gaBds[16];

// EZMIDI 0x85f0
struct BankHeader *gaHds[16];

// EZMIDI 0x7c58
struct PanPair pan_2_vol[128];

// EZMIDI 0x8158
short chr_curve[512];

// EZMIDI 0x9c4
void _init_channels(void) {
    int nChannel;

    for (nChannel = 0; nChannel < 16; ++nChannel) {
        struct MidiChannel *pChannel = &gChan[nChannel];

        pChannel->mUnknown00 = 0;
        pChannel->mVolume = 0x64;
        pChannel->mPan = 0x40;
        pChannel->mExpression = 0x7f;
        pChannel->mUnknown04 = 0;
        pChannel->mUnknown06 = 0;
        pChannel->mUnknown08 = 0;
        pChannel->mUnknown0A = 0xff;
        pChannel->mUnknown0B = 0;
        pChannel->mUnknown0C = 0x40;
    }
}

// EZMIDI 0xb4c
void _init_banks(void) {
    int nBank;

    for (nBank = 0; nBank < 16; ++nBank) {
        gaBds[nBank] = 0;
        gaHds[nBank] = 0;
    }
}

// EZMIDI 0x54c
void _build_pantable(void) {
    int nIndex;

    for (nIndex = 0; nIndex < 64; ++nIndex) {
        int nLeft = 0x8000 - (nIndex << 7);
        int nRight = (nIndex * 3) << 7;

        pan_2_vol[nIndex].mLeft = nLeft;
        pan_2_vol[nIndex].mRight = nRight;
        pan_2_vol[0x7f - nIndex].mRight = nLeft;
        pan_2_vol[0x7f - nIndex].mLeft = nRight;
    }
}

// EZMIDI 0x668
void _build_chorus(int nDepth) {
    int nIndex;

    (void)nDepth; // Yes, the binary takes a size and never reads it.

    for (nIndex = 0; nIndex < 129; ++nIndex) {
        int nValue = (nIndex << 15) - nIndex;

        if (nValue < 0) {
            nValue += 0x7f;
        }
        chr_curve[nIndex] = (short)(nValue >> 7);
    }
    for (; nIndex < 257; ++nIndex) {
        // Yes, the binary reads below the table for all but the last step.
        chr_curve[nIndex] = chr_curve[nIndex - 256];
    }
    for (; nIndex < 385; ++nIndex) {
        chr_curve[nIndex] = (short)-chr_curve[nIndex - 256];
    }
    for (; nIndex < 512; ++nIndex) {
        chr_curve[nIndex] = chr_curve[0x300 - nIndex];
    }
}

// EZMIDI 0x1aa4
void CheckEffBits(int nMode, unsigned int *pWords, int nIndex, int nUnused) {
    unsigned int nMask = slot_2_mask.mWords[nIndex];
    unsigned int nBits = pWords[nIndex & 1] & nMask;

    (void)nUnused; // Yes, the binary takes a fourth argument and never reads it.
    if (nMode == 0) {
        if (nBits == 0) {
            return;
        }
        pWords[nIndex & 1] &= ~nMask;
    } else {
        if (nBits != 0) {
            return;
        }
        pWords[nIndex & 1] |= nMask;
    }
}

// EZMIDI 0x1c44
int _note_2_pitch(int nNote, int nFine, int nTune, int nScale) {
    int nPitch = sceSdNote2Pitch(nNote, nFine, nTune, 0) & 0xffff;

    // The binary divides through the multiply-high idiom; 48000 is the exact divisor.
    nPitch = nPitch * nScale / 48000;
    if (nPitch >= 0x4000) {
        nPitch = 0x3fff;
    }
    return nPitch;
}
