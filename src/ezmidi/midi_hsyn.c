#include "ezmidi/synth.h"

#include "ezmidi/imports.h"

// EZMIDI 0x6ec0
unsigned char gRemixMode;

// EZMIDI 0x6ec1
unsigned char gMonoMode;

// EZMIDI 0x6e90
int gPauseCount;

// EZMIDI 0x6ebc
unsigned short gSynthRun;

// EZMIDI 0x7e58
int gFixedGainA;

// EZMIDI 0x7e5c
int gFixedGainB;

// EZMIDI 0x6ea8
int gTuneAlt0;

// EZMIDI 0x6eac
int gTuneAlt1;

// EZMIDI 0x6ff0
unsigned int gReg_VMixR[2];

// EZMIDI 0x7010
unsigned int gReg_VMixEL[2];

// EZMIDI 0x7028
unsigned int gReg_VMixER[2];

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

/**
 * Scale through the volume curve idiom.
 *
 * Reproduces the binary's multiply-high sequence bit for bit.
 */
static int ScaleCurve(int nValue) {
    long long nWide = (long long)nValue * 0x81020409LL;
    int nHigh = (int)(nWide >> 32);

    return ((nHigh + nValue) >> 6) - (nValue < 0 ? 1 : 0);
}

/**
 * Scale into fifteen bits with symmetric rounding.
 */
static int MulShr15(int nValue, int nGain) {
    int nProduct = nValue * nGain;

    if (nProduct < 0) {
        nProduct += 0x7fff;
    }
    return nProduct >> 15;
}

// EZMIDI 0x1d14
int _apply_channel_to_note(struct Note *pNote, int nApply) {
    int nPan = pNote->mPan;
    int nSaved0C = pNote->mUnknown0C;
    int nSaved14 = pNote->mUnknown14;
    int nVolume = ScaleCurve((unsigned short)ScaleCurve(pNote->mUnknown06 * gChan[pNote->mChannel].mVolume) *
                             gChan[pNote->mChannel].mExpression);
    unsigned short nVoice;

    if ((pNote->mFlags & 2) != 0) {
        pNote->mUnknown0C = pNote->mUnknown08;
    }
    if (gPauseCount <= 0) {
        nVoice = pNote->mUnknown0C;
    } else if (((unsigned int)gSynthRun >> pNote->mChannel & 1) == 0) {
        nVoice = 0;
    } else {
        nVoice = pNote->mUnknown0C;
    }
    if (gMonoMode == 0) {
        if (gRemixMode != 0 && (pNote->mUnknown05 & 1) == 0) {
            nPan = 0x40;
        }
        if ((pNote->mUnknown05 & 4) != 0) {
            if (pNote->mUnknown03 == 0xff) {
                nPan = 0x7f;
                pNote->mUnknown12 = 0;
                pNote->mUnknown14 = (unsigned short)MulShr15(nVolume, gFixedGainB);
            } else {
                nPan = 0;
                pNote->mUnknown12 = (unsigned short)MulShr15(nVolume, gFixedGainA);
                pNote->mUnknown14 = 0;
            }
        } else {
            pNote->mUnknown12 = (unsigned short)MulShr15(nVolume, pan_2_vol[nPan].mLeft);
            pNote->mUnknown14 = (unsigned short)MulShr15(nVolume, pan_2_vol[nPan].mRight);
        }
    } else {
        pNote->mUnknown12 = (unsigned short)MulShr15(nVolume, pan_2_vol[nPan].mLeft);
        pNote->mUnknown14 = (unsigned short)MulShr15(nVolume, pan_2_vol[nPan].mRight);
    }
    if ((pNote->mFlags & 2) != 0) {
        pNote->mUnknown0E = pNote->mUnknown12;
        pNote->mUnknown10 = pNote->mUnknown14;
    }
    if (nApply == 0) {
        return 0;
    }
    sceSdSetParam((pNote->mUnknown02 | 0x100) & 0xffff, pNote->mUnknown10);
    sceSdSetParam((pNote->mUnknown02 | 0x200) & 0xffff, nVoice);
    if (pNote->mUnknown0C != nSaved0C || pNote->mUnknown14 != nSaved14) {
        return 1;
    }
    return 0;
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

// EZMIDI 0x2240
int _fire_off_sample(int nSample, int nChannel, int nKey, int nScale, const struct NoteEvent *pEvent,
                     const struct NoteEvent *pExtra, int nMode) {
    int nFine = 0;
    int nTune = 0;
    int nPan = 0x40;
    int nGroup = 0xa;
    int nFlags = 3;
    struct OffsetTable *pSamples = (struct OffsetTable *)((char *)gpHd + gpHd->mSampleTab0);
    struct OffsetTable *pSamples2 = (struct OffsetTable *)((char *)gpHd + gpHd->mSampleTab1);
    struct Sample *pSample;
    struct SampleDesc *pDesc;
    unsigned short nLevel;
    int nPitch;
    int nPitch2;
    int nEff;
    int nDataAddr;
    int nSlot;
    struct Note *pNote;

    if (nSample > pSamples->mCount) {
        return -1;
    }
    pSample = (struct Sample *)((char *)pSamples + pSamples->mOffsets[nSample]);
    nEff = pSample->mUnknown29;
    if (gRemixMode != 0) {
        nEff = (nEff & 0x40) | 3;
    }
    nLevel = pSample->mUnknown10 * nScale;
    nLevel = ScaleCurve(nLevel * pEvent->mUnknown06);
    if (pExtra != 0) {
        nFine += pExtra->mUnknown12;
        nTune += pExtra->mUnknown13;
        nLevel = ScaleCurve(nLevel * pExtra->mUnknown10);
        nPan = pExtra->mUnknown11;
    }
    nGroup = gChan[nKey].mUnknown0C;
    pDesc = (struct SampleDesc *)((char *)pSamples2 + pSamples2->mOffsets[pSample->mUnknown00]);
    nDataAddr = (int)(long)((char *)gpBd + pDesc->mDataOff);
    if (pDesc->mUnknown06 == 1) {
        nFlags |= 4;
    }
    nFine += pEvent->mUnknown08;
    nTune += pEvent->mUnknown09;
    if (pEvent->mUnknown07 != 0x40) {
        nPan = pEvent->mUnknown07;
    }
    if (pSample->mUnknown0D != 0x40) {
        nPan = pSample->mUnknown0D;
    }
    nPitch = _note_2_pitch(pSample->mUnknown0B, nChannel + nFine, nTune, pDesc->mUnknown04);
    nPitch2 = _note_2_pitch(pSample->mUnknown0B, nChannel + nFine,
                            nTune + ((nMode & 1) != 0 ? gTuneAlt1 : gTuneAlt0), pDesc->mUnknown04);
    nSlot = _search_for_slot(nGroup, nGroup, nLevel);
    if (nSlot == -1) {
        return -2;
    }
    pNote = _new_note();
    pNote->mUnknown02 = nSample;
    pNote->mChannel = nChannel;
    pNote->mUnknown01 = nKey;
    pNote->mFlags = nFlags;
    pNote->mUnknown03 = (nMode & 1) != 0 ? 0xff : 0;
    pNote->mUnknown08 = nPitch;
    pNote->mUnknown06 = nLevel;
    pNote->mPan = nPan;
    pNote->mUnknown20 = (gChan[nKey].mUnknown04 & 0xff) - 1;
    pNote->mUnknown16 = 0;
    pNote->mUnknown1E = 0;
    pNote->mUnknown1F = 0;
    pNote->mUnknown21 = gChan[nKey].mUnknown04 & 0xff;
    pNote->mUnknown18 = nPitch - nPitch2;
    if ((gChan[nKey].mUnknown0B & 1) != 0) {
        pNote->mUnknown05 |= 1;
    }
    _apply_channel_to_note(pNote, 1);
    sceSdSetParam((nSlot | 0x300) & 0xffff, pSample->mUnknown12);
    sceSdSetParam((nSlot | 0x400) & 0xffff, pSample->mUnknown14);
    sceSdSetAddr((nSlot | 0x2040) & 0xffff, nDataAddr);
    CheckEffBits(nEff & 1, &voice_alloc[14], nSlot, 0x1800);
    CheckEffBits(nEff & 2, gReg_VMixR, nSlot, 0x1a00);
    CheckEffBits(nEff & 4, gReg_VMixEL, nSlot, 0x1900);
    CheckEffBits(nEff & 8, gReg_VMixER, nSlot, 0x1b00);
    do_kOn(nSlot);
    voice_alloc[nSlot] |= slot_2_mask.mWords[nSlot];
    return (int)(pNote - gCurrentNotes);
}
