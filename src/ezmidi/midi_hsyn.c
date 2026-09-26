#include "ezmidi/synth.h"

#include "ezmidi/imports.h"

// EZMIDI 0x6ec0
unsigned char gRemixMode;

// EZMIDI 0x6ec1
unsigned char gMonoMode;

// EZMIDI 0x6e90
int gPauseCount;

// EZMIDI 0x6e94
unsigned int gUpdateMask;

// EZMIDI 0x6eb8
unsigned short gPauseKeepMask;

// EZMIDI 0x6ebc
unsigned short gSynthRun;

// EZMIDI 0x6ebe
unsigned short gChorusAltStep;

// EZMIDI 0x6ea0
int gRunDivisor = 0xc800;

// EZMIDI 0x6ea4
int gAltDivisor = 0x12c;

// EZMIDI 0x6e9a
unsigned short gFadeStepMin = 0x1800;

// EZMIDI 0x6e9c
unsigned char gFadeTableIdx = 2;

// EZMIDI 0x6ed4
int gFadeSnap = 0x200;

// EZMIDI 0x6ed8
unsigned short gFadeTable[6] = {0x08, 0x11, 0x1b, 0x1f, 0x25, 0x00};

// EZMIDI 0x6ee4
int gFadeMode = 6;

// EZMIDI 0x6ee8
int gFadeTimed = 1;

// EZMIDI 0x6eec
unsigned short gVoiceParamBase[2] = {0x0600, 0x0700};

// EZMIDI 0x6ef0
int gLinValTable[59] = {
    655320, 546100, 468085, 364066, 327660, 273050, 218440, 182033, 156028, 136525, 112986, 91016,
    79917,  68262,  56493,  44884,  39477,  33779,  27305,  21844,  19274,  17245,  14246,  11298,
    9929,   8401,   7123,   5649,   4964,   4255,   3523,   2730,   2520,   2184,   1724,   1424,
    1213,   1056,   885,    712,    618,    528,    442,    352,    297,    273,    218,    172,
    156,    131,    109,    88,     78,     65,     55,     44,     38,     33,     27,
};

// EZMIDI 0x37fc
int _pick_lin_val(int nAbs, unsigned char *pTimer) {
    int nLo = 0;
    int nHi = 58;
    int nMid;

    if ((unsigned int)(gLinValTable[nLo] * 30) < (unsigned int)nAbs) {
        *pTimer = (unsigned char)((unsigned int)nAbs / (unsigned int)gLinValTable[nLo]);
        return nLo;
    }
    if ((unsigned int)nAbs < (unsigned int)(gLinValTable[nHi] * 30)) {
        *pTimer = (unsigned char)((unsigned int)nAbs / (unsigned int)gLinValTable[nHi]);
        return nHi;
    }
    for (;;) {
        nMid = (nLo + nHi) / 2;
        if ((unsigned int)(gLinValTable[nMid] * 30) < (unsigned int)nAbs) {
            nHi = nMid;
        } else {
            nLo = nMid;
        }
        if (nLo + 1 >= nHi) {
            break;
        }
    }
    *pTimer = (unsigned char)((unsigned int)nAbs / (unsigned int)(gLinValTable[nHi] * 2));
    return nHi;
}

// EZMIDI 0x7e58
int gFixedGainA;

// EZMIDI 0x7e5c
int gFixedGainB;

// EZMIDI 0x6ea8
int gTuneAlt0 = 30;

// EZMIDI 0x6eac
int gTuneAlt1 = 20;

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

// EZMIDI 0x6ec4
struct BankHeader *gpHd;

// EZMIDI 0x6ec8
struct BankData *gpBd;

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

        pChannel->mProgram = 0;
        pChannel->mVolume = 0x64;
        pChannel->mPan = 0x40;
        pChannel->mExpression = 0x7f;
        pChannel->mBank = 0;
        pChannel->mUnknown06 = 0;
        pChannel->mUnknown08 = 0;
        pChannel->mBankMsb = 0xff;
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

// EZMIDI 0xbe4
void hs_prog_change(int nChannel, int nProgram) {
    gChan[nChannel].mProgram = (unsigned char)nProgram;
}

// EZMIDI 0x470
void _build_slotmask(void) {
    int nGroup;
    int nSlot;

    for (nGroup = 0; nGroup < 2; ++nGroup) {
        for (nSlot = 0; nSlot < 24; ++nSlot) {
            slot_2_mask.mWords[nGroup | (nSlot << 1)] = 1 << nSlot;
        }
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
int _fire_off_sample(int nSample, int nNote, int nChannel, int nVelocity, const struct NoteEvent *pEvent,
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
    nLevel = pSample->mUnknown10 * nVelocity;
    nLevel = ScaleCurve(nLevel * pEvent->mUnknown06);
    if (pExtra != 0) {
        nFine += pExtra->mUnknown12;
        nTune += pExtra->mUnknown13;
        nLevel = ScaleCurve(nLevel * pExtra->mUnknown10);
        nPan = pExtra->mUnknown11;
    }
    nGroup = gChan[nChannel].mUnknown0C;
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
    nPitch = _note_2_pitch(pSample->mUnknown0B, nNote + nFine, nTune, pDesc->mUnknown04);
    nPitch2 = _note_2_pitch(pSample->mUnknown0B, nNote + nFine,
                            nTune + ((nMode & 1) != 0 ? gTuneAlt1 : gTuneAlt0), pDesc->mUnknown04);
    nSlot = _search_for_slot(nGroup, nGroup, nLevel);
    if (nSlot == -1) {
        return -2;
    }
    pNote = _new_note();
    pNote->mUnknown02 = nSample;
    pNote->mNote = nNote;
    pNote->mChannel = nChannel;
    pNote->mFlags = nFlags;
    pNote->mUnknown03 = (nMode & 1) != 0 ? 0xff : 0;
    pNote->mUnknown08 = nPitch;
    pNote->mUnknown06 = nLevel;
    pNote->mPan = nPan;
    pNote->mUnknown20 = (gChan[nChannel].mBank & 0xff) - 1;
    pNote->mUnknown16 = 0;
    pNote->mUnknown1E = 0;
    pNote->mUnknown1F = 0;
    pNote->mBank = (unsigned char)(gChan[nChannel].mBank & 0xff);
    pNote->mUnknown18 = nPitch - nPitch2;
    if ((gChan[nChannel].mUnknown0B & 1) != 0) {
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


// EZMIDI 0x2c6c
int hs_note_on(int nChannel, int nNote, int nVelocity) {
    int nSlot = -1;
    const unsigned char *pProg = 0;
    const unsigned char *pArt = 0;
    struct OffsetTable *pTab0;
    struct OffsetTable *pTab1;
    int nEntry;
    int nCount;
    int nIndex;
    int nResult2;
    int nSamp2;
    const unsigned char *pVoice2;
    unsigned short nArt;

    if (nChannel < 0 || nChannel >= 16) {
        return -1;
    }
    if (gChan[nChannel].mBank >= 0x11) {
        return -2;
    }
    gpHd = gaHds[gChan[nChannel].mBank];
    if (gpHd == 0 || gpBd == 0) {
        return -3;
    }
    pTab0 = (struct OffsetTable *)((char *)gpHd + gpHd->mProgTab0);
    pTab1 = (struct OffsetTable *)((char *)gpHd + gpHd->mProgTab1);
    if (pTab0->mCount < gChan[nChannel].mProgram) {
        return -4;
    }
    if (gpHd == 0 || gpBd == 0) {
        return -5;
    }
    nEntry = pTab0->mOffsets[gChan[nChannel].mProgram];
    if (nEntry == -1) {
        return -6;
    }
    pProg = (const unsigned char *)pTab0 + nEntry;
    nCount = pProg[4];
    for (nIndex = 0; nIndex < nCount; ++nIndex) {
        if (nNote < pProg[nIndex * 20 + 0x26]) {
            continue;
        }
        if (pProg[nIndex * 20 + 0x28] < nNote) {
            continue;
        }
        pArt = pProg + nIndex * 20 + 0x24;
        break;
    }
    if (pArt == 0) {
        return -7;
    }
    nArt = pArt[0] | ((unsigned short)pArt[1] << 8);
    if ((short)nArt == -1) {
        return -8;
    }
    pVoice2 = (const unsigned char *)pTab1 + pTab1->mOffsets[nArt];
    nSamp2 = pVoice2[4] | ((unsigned short)pVoice2[5] << 8);
    if (nSamp2 == -1) {
        return -9;
    }
    nSlot = _fire_off_sample(nSamp2, nNote, nChannel, nVelocity, (const struct NoteEvent *)pProg,
                             (const struct NoteEvent *)pArt, 0);
    if (nSlot < 0) {
        return nSlot;
    }
    if ((gChan[nChannel].mUnknown0B & 4) == 0) {
        return nSlot;
    }
    nResult2 = _fire_off_sample(nSamp2, nNote, nChannel, nVelocity, (const struct NoteEvent *)pProg,
                                (const struct NoteEvent *)pArt, 1);
    gCurrentNotes[nSlot].mUnknown02 = (unsigned char)nResult2;
    return nSlot;
}

// EZMIDI 0x3250
int hs_kill_idx(struct Note *pNote, int nUnused) {
    int nVoice = pNote->mUnknown02;

    (void)nUnused; // Yes, the binary takes a second argument and never reads it.
    if (nUnused == 0) {
        if ((pNote->mFlags & 8) == 0) {
            do_kOff(nVoice);
            pNote->mFlags |= 8;
        }
    }
    sceSdSetParam(nVoice, 0);
    sceSdSetParam((nVoice | 0x100) & 0xffff, 0);
    voice_alloc[nVoice] &= ~slot_2_mask.mWords[nVoice];
    return _free_note(pNote, 0);
}

// EZMIDI 0x33cc
int hs_idx_off(int nIndex) {
    if ((gCurrentNotes[nIndex].mFlags & 1) == 0) {
        return -1;
    }
    do_kOff(gCurrentNotes[nIndex].mUnknown02);
    gCurrentNotes[nIndex].mFlags = gCurrentNotes[nIndex].mUnknown02 | 8;
    return 0;
}

// EZMIDI 0x34e0
int hs_note_off(int nChannel, int nNote) {
    int nIndex = _find_note(nChannel, nNote);

    if (nIndex == -1) {
        return -1;
    }
    hs_idx_off(nIndex);
    if (gCurrentNotes[nIndex].mUnknown03 == 0) {
        return 0;
    }
    if (gCurrentNotes[nIndex].mUnknown03 == 0xff) {
        return 0;
    }
    hs_idx_off(gCurrentNotes[nIndex].mUnknown03);
    gCurrentNotes[nIndex].mUnknown03 = 0;
    // Yes, the binary rereads the cleared link, so this always parks index zero.
    gCurrentNotes[gCurrentNotes[nIndex].mUnknown03].mUnknown03 = 0;
    return 0;
}

// EZMIDI 0x3654
int hs_check_playing(struct Note *pNote) {
    int nVoice = pNote->mUnknown02;
    int nDead;

    if ((pNote->mFlags & 2) != 0) {
        pNote->mFlags &= 0xfd;
        return 0;
    }
    nDead = 0;
    if ((pNote->mFlags & 8) != 0) {
        nDead = 1;
    } else if ((pNote->mFlags & 4) == 0 &&
               (voice_alloc[12 + (nVoice & 1)] & (unsigned int)slot_2_mask.mWords[nVoice]) != 0) {
        nDead = 1;
    }
    if (nDead == 0) {
        return 0;
    }
    if ((sceSdGetParam((nVoice | 0x500) & 0xffff) & 0xffff) != 0) {
        return 0;
    }
    pNote->mFlags |= 8;
    hs_kill_idx(pNote, 1);
    return -1;
}

// EZMIDI 0x3a70
int _move_vol_towards(struct Note *pNote, int nMode) {
    // The mode picks a gain pair: mode 0 moves +0x0E towards +0x12, mode 1
    // moves +0x10 towards +0x14.
    unsigned short *pGains = (unsigned short *)pNote + nMode;
    unsigned char *pTimer = (unsigned char *)pNote + nMode;
    int nDiff = 0;
    int nResult = 0;
    int nDir;
    int nStep;
    int nAbs;

    if ((gChan[pNote->mChannel].mUnknown0B & 2) == 0) {
        if (pGains[7] == pGains[9]) {
            return 0;
        }
        pGains[7] = pGains[9];
        return 1;
    }
    if (pTimer[0x1E] != 0 && --pTimer[0x1E] == 0) {
        nResult = 1;
        pGains[9] = pGains[13];
    }
    if ((pGains[9] & 0x8000) == 0 && (pGains[7] & 0x8000) == 0) {
        nDiff = (int)pGains[9] - (int)pGains[7];
    } else if (pGains[7] == pGains[9]) {
        nDiff = 0;
    } else {
        nResult = (pGains[9] & 0x8000) != 0 ? 2 : 1;
    }
    if (nResult != 0) {
        pGains[7] = (unsigned short)sceSdGetParam((pNote->mUnknown02 | gVoiceParamBase[nMode]) & 0xffff);
        if (nResult == 2) {
            pGains[9] = pGains[7];
        }
        nDiff = (int)pGains[9] - (int)pGains[7];
    }
    if (nDiff == 0) {
        return 0;
    }
    if (gFadeMode == 5) {
        pGains[7] = pGains[9];
        return 1;
    }
    if (gFadeMode != 6) {
        nAbs = nDiff >= 0 ? nDiff : -nDiff;
        if ((int)gFadeStepMin < nAbs) {
            if ((pGains[7] & 0xa000) == 0xa000) {
                nDir = 1;
            } else if ((pGains[7] & 0xa000) == 0x8000) {
                nDir = -1;
            } else if (pGains[9] >= 0x801) {
                nDir = 1;
            } else {
                nDir = -1;
            }
            if (gFadeTimed != 0) {
                if (nDir != -1 || pGains[9] >= 0x100) {
                    pTimer[0x1E] = 0x0f;
                    pGains[13] = pGains[9];
                    nStep = _pick_lin_val(nAbs, &pTimer[0x1E]);
                } else {
                    pGains[7] = (unsigned short)(gFadeTable[gFadeMode] | 0xa000);
                    return 1;
                }
            } else {
                nStep = gFadeTable[gFadeTableIdx];
            }
            if (nDir == 1) {
                pGains[7] = (unsigned short)(nStep | 0x8000);
            } else {
                pGains[7] = (unsigned short)(nStep | 0xa000);
            }
            pGains[9] = pGains[7];
            return 1;
        }
    }
    nAbs = nDiff >= 0 ? nDiff : -nDiff;
    if (nAbs < gFadeSnap) {
        pGains[7] = pGains[9];
        return 1;
    }
    if (nDiff > 0) {
        pGains[7] += (unsigned short)gFadeSnap;
    } else {
        pGains[7] -= (unsigned short)gFadeSnap;
    }
    return 1;
}

// EZMIDI 0x4334
int _apply_chorus(int nCurrent, int nRate, int nTarget, unsigned short *pState) {
    *pState = (unsigned short)(*pState + nRate);
    return (nCurrent + MulShr15(nTarget, chr_curve[*pState >> 7])) & 0xffff;
}

// EZMIDI 0x43f8
int hs_update_note_and_fx(struct Note *pNote) {
    int nCount = 0;
    int nValue;
    int nAlt;

    if ((gUpdateMask >> pNote->mChannel & 1) != 0) {
        nCount += _apply_channel_to_note(pNote, 0);
    }
    if (gPauseCount > 0 && (gPauseKeepMask >> pNote->mChannel & 1) == 0) {
        nValue = 0;
    } else if ((pNote->mUnknown05 & 4) != 0) {
        nAlt = pNote->mUnknown03 == 0xff ? 1 : 0;
        // Yes, the binary reads the halfword after the run bits for the alternate step.
        nValue = (unsigned short)_apply_chorus(pNote->mUnknown0C, nAlt != 0 ? gChorusAltStep : gSynthRun,
                                               pNote->mUnknown18, &pNote->mUnknown16);
        ++nCount;
    } else {
        nValue = pNote->mUnknown0C;
    }
    nCount += _move_vol_towards(pNote, 0);
    nCount += _move_vol_towards(pNote, 1);
    if (nCount != 0) {
        sceSdSetParam(pNote->mUnknown02, pNote->mUnknown0E);
        sceSdSetParam((pNote->mUnknown02 | 0x100) & 0xffff, pNote->mUnknown10);
        sceSdSetParam((pNote->mUnknown02 | 0x200) & 0xffff, (unsigned short)nValue);
    }
    return 0;
}

// EZMIDI 0x4654
int hs_reapply_channel(int nChannel) {
    int nIndex;

    for (nIndex = 0; nIndex < 50; ++nIndex) {
        if ((gCurrentNotes[nIndex].mFlags & 1) == 0) {
            continue;
        }
        if (nChannel != -1 && gCurrentNotes[nIndex].mChannel != nChannel) {
            continue;
        }
        _apply_channel_to_note(&gCurrentNotes[nIndex], 1);
    }
    return 0;
}

// EZMIDI 0x477c
int ShowSynthState(int nUnknown) {
    (void)nUnknown; // Yes, the binary takes a value and never reads it.
    return 0;
}

// EZMIDI 0x47ac
void ResetSynthState(void) {
    int nChannel;

    for (nChannel = 0; nChannel < 16; ++nChannel) {
        hs_prog_change(nChannel, 0);
    }
    voice_alloc[1] = 0;
    voice_alloc[0] = 0;
    _init_channels();
    _init_banks();
}
