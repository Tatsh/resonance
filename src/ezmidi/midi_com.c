#include "ezmidi/common.h"

#include "ezmidi/ezmidi.h"
#include "ezmidi/imports.h"
#include "ezmidi/synth.h"

// This file mirrors the original `midi_com.c`: common support around the engine.

// EZMIDI 0x6e84
int gTickTrace;

// EZMIDI 0x6e50
const char gTickPre[] = "]";

// EZMIDI 0x6e54
const char gTickPost[] = "[";

// EZMIDI 0x6e40
const char gScanFmt[] = "%d midi bytes\n";

// EZMIDI 0x8564
int gTickThread;

// EZMIDI 0x8558
struct TimerState gTimerState;

// EZMIDI 0x7040
struct InBuffer gInBuf[2];

// EZMIDI 0x7840
struct InBuffer gStagedBuf;

// EZMIDI 0x4bf4
void HardSynthAllNotesOff(int nChannel, int nReset) {
    int nIndex;

    if (nReset != 0) {
        hs_tick_setup();
    }
    for (nIndex = 0; nIndex < 50; ++nIndex) {
        if ((gCurrentNotes[nIndex].mFlags & 1) == 0) {
            continue;
        }
        if (nChannel != -1 && gCurrentNotes[nIndex].mChannel != nChannel) {
            continue;
        }
        hs_kill_idx(&gCurrentNotes[nIndex], 0);
    }
    if (nChannel == -1 && _count_notes() != 0) {
        ShowSynthState(0xff);
    }
    if (nReset == 0) {
        return;
    }
    _do_reg_out();
}

// EZMIDI 0x4d70
unsigned char *HandleMidiMessage(unsigned char *pMsg) {
    int nChannel = pMsg[0] & 0xf;
    int nLen = 3;

    switch (pMsg[0] & 0xf0) {
    case 0x80:
        hs_note_off(nChannel, pMsg[1]);
        nLen = 2;
        break;
    case 0x90:
        hs_note_on(nChannel, pMsg[1], pMsg[2]);
        break;
    case 0xa0:
        break;
    case 0xb0: {
        unsigned int nController = pMsg[1];

        if (nController >= 0x7c) {
            break;
        }
        switch (nController) {
        case 0:
            gChan[nChannel].mBankMsb = pMsg[2];
            break;
        case 1:
            gChan[nChannel].mUnknown06 = pMsg[2];
            gUpdateMask |= (unsigned int)1 << nChannel;
            break;
        case 7:
            gChan[nChannel].mVolume = pMsg[2];
            gUpdateMask |= (unsigned int)1 << nChannel;
            break;
        case 10:
            gChan[nChannel].mPan = pMsg[2];
            gUpdateMask |= (unsigned int)1 << nChannel;
            break;
        case 11:
            gChan[nChannel].mExpression = pMsg[2];
            gUpdateMask |= (unsigned int)1 << nChannel;
            break;
        case 32:
            gChan[nChannel].mBank = (unsigned short)((gChan[nChannel].mBankMsb << 7) + pMsg[2]);
            gChan[nChannel].mBankMsb = 0;
            break;
        case 80:
            if (pMsg[2] >= 0x40) {
                gChan[nChannel].mUnknown0B |= 2;
            } else {
                gChan[nChannel].mUnknown0B &= 0xfd;
            }
            gUpdateMask |= (unsigned int)1 << nChannel;
            break;
        case 81:
            if (pMsg[2] >= 0x40) {
                gChan[nChannel].mUnknown0B |= 4;
            } else {
                gChan[nChannel].mUnknown0B &= 0xfb;
            }
            gUpdateMask |= (unsigned int)1 << nChannel;
            break;
        case 82:
            if (pMsg[2] >= 0x40) {
                gChan[nChannel].mUnknown0B |= 0x20;
            } else {
                gChan[nChannel].mUnknown0B &= 0xdf;
            }
            gUpdateMask |= (unsigned int)1 << nChannel;
            break;
        case 83:
            if (pMsg[2] >= 0x40) {
                gChan[nChannel].mUnknown0B |= 0x10;
            } else {
                gChan[nChannel].mUnknown0B &= 0xef;
            }
            gUpdateMask |= (unsigned int)1 << nChannel;
            break;
        case 88:
            if (pMsg[2] < 3) {
                gChan[nChannel].mUnknown0C = pMsg[2];
            } else {
                gChan[nChannel].mUnknown0C = (unsigned char)(pMsg[2] * 48 + 16);
            }
            break;
        case 89:
            if (pMsg[2] >= 0x40) {
                gChan[nChannel].mUnknown0B |= 1;
            } else {
                gChan[nChannel].mUnknown0B &= 0xfe;
            }
            break;
        case 121:
            break;
        case 123:
            HardSynthAllNotesOff(nChannel, 0);
            break;
        default:
            break;
        }
        break;
    }
    case 0xc0:
        hs_prog_change(nChannel, pMsg[1]);
        nLen = 2;
        break;
    case 0xd0:
        nLen = 2;
        break;
    case 0xe0:
        gChan[nChannel].mUnknown08 = (unsigned short)((pMsg[1] << 7) + pMsg[2]);
        gUpdateMask |= (unsigned int)1 << nChannel;
        break;
    default:
        break;
    }
    return pMsg + nLen;
}

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

// EZMIDI 0x5d34
int HardSynthParseNew(unsigned char *pData, int nCount, int nBuffer) {
    unsigned char *pMsg = pData;

    (void)nBuffer; // Yes, the binary takes a buffer index and never reads it.
    while (pMsg < pData + nCount) {
        pMsg = HandleMidiMessage(pMsg);
        if (pMsg == 0) {
            return -1;
        }
    }
    return 0;
}

// EZMIDI 0x5e00
int scan_inbuf(int nBuffer) {
    struct InBuffer *pBuf = &gInBuf[nBuffer];
    int nCount = pBuf->mCount;

    if (nCount > 0) {
        if (gTickTrace != 0) {
            printf(gScanFmt, nCount);
        }
        memcpy(&gStagedBuf, pBuf, (unsigned int)(nCount + 8));
        pBuf->mCount = 0;
        HardSynthParseNew(gStagedBuf.mData, gStagedBuf.mCount, nBuffer);
    }
    gStagedBuf.mCount = 0;
    return nCount;
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

// EZMIDI 0x6054
int make_thread(void) {
    struct ThreadParam param;

    param.mAttr = 0x02000000;
    param.mOption = 0;
    param.mEntry = hsyn_atick;
    param.mStackSize = 0x800;
    param.mPriority = 0x1d;
    return CreateThread(&param);
}

// EZMIDI 0x60c8
int set_timer(struct TimerState *pTimer) {
    unsigned int aClock[2];
    int nAlarm;

    USec2SysClock(0x823, aClock);
    pTimer->mClock = (int)aClock[0];
    nAlarm = AllocHardTimer(1, 0x20, 1);
    if (nAlarm <= 0) {
        return -1;
    }
    pTimer->mTimer = nAlarm;
    if (SetTimerHandler(nAlarm, (unsigned int)pTimer->mClock, 0x5fec, pTimer) != 0) {
        return -2;
    }
    if (SetupHardTimer(nAlarm, 1, 0, 1) != 0) {
        return -3;
    }
    return 0;
}

// EZMIDI 0x620c
int start_timer(struct TimerState *pTimer) {
    if (StartHardTimer(pTimer->mTimer) != 0) {
        return -1;
    }
    return 0;
}

// EZMIDI 0x6800
int HardSynthInit(void) {
    _init_channels();
    _init_banks();
    _build_slotmask();
    _build_pantable();
    _build_chorus(0x400);
    HardSynthConfig(0);
    gTickThread = make_thread();
    gTimerState.mThread = gTickThread;
    StartThread(gTickThread, 0);
    set_timer(&gTimerState);
    HardSynthReset();
    start_timer(&gTimerState);
    return (int)gInBuf;
}

// EZMIDI 0x68d0
int HardSynthReset(void) {
    ResetSynthState();
    return 0;
}

// EZMIDI 0x6660
int HardSynthSetRemix(int nMode) {
    gRemixMode = nMode != 0;
    return 0;
}

// EZMIDI 0x66a4
int HardSynthSetMono(int nMode) {
    gMonoMode = nMode != 0;
    gUpdateMask = 0xffff;
    return 0;
}

// EZMIDI 0x66f4
void HardSynthConfig(const void *pConfig) {
    if (pConfig != 0) {
        const struct SynthConfig *pBlock = (const struct SynthConfig *)pConfig;

        gFadeStepMin = pBlock->mFadeStepMin;
        gFadeTableIdx = pBlock->mFadeTableIdx;
        gRunDivisor = pBlock->mRunDivisor;
        gAltDivisor = pBlock->mAltDivisor;
        gTuneAlt0 = pBlock->mTuneAlt0;
        gTuneAlt1 = pBlock->mTuneAlt1;
        gPauseKeepMask = pBlock->mPauseKeepMask;
    }
    gSynthRun = (unsigned short)(0x8230000 / (1000 * (gRunDivisor + 1)));
    gChorusAltStep = (unsigned short)(0x8230000 / (1000 * (gAltDivisor + 1)));
}
