#include <stddef.h>

#include <csl.h>
#include <kernel.h>
#include <libsd.h>
#include <stdio.h>
#include <sysclib.h>

#include "ezmidi/hsyn.h"

enum {
    kSlotCoreMask = 1, // A voice slot is SD_VOICE(voice) | core.
    kAnyCore = -1,
    kAllChannels = -1,
    kNoSlot = -1,
    kNoNote = -1,

    kPanPositions = 128,
    kPanFullGain = 0x8000,
    kPanNearStep = 128,
    kPanFarStep = 384,
    kSlotCodes = 64,

    kChorusCurveLength = 512,
    kChorusQuarterWave = kChorusCurveLength / 4,
    kChorusThreeQuarterWave = 3 * kChorusQuarterWave,
    kChorusPeak = 32767,
    kChorusPositionShift = 7, // Right shift from a chorus position to its curve index.

    kDefaultChannelVolume = 100,
    kDefaultChannelPan = 64,
    kDefaultChannelExpression = 127,
    kDefaultChannelBankHi = 255,
    kDefaultChannelPriority = 64,

    kMaxNotePriority = 127,
    kReleasedPriorityPenalty = 10,
    kStealPriority = 10,
    kDefaultVoicePriority = 10,

    kClockUseShift = 16,

    kSpuSampleRate = 48000,
    kMaxPitch = 0x3fff,

    kMidiFullScale = 127,
    kMidiPanLeft = 0,
    kMidiPanCentre = 64,
    kMidiPanRight = 127,
    kQ15One = 32768, // Divisor of the Q15 pan, chorus, and volume factors.
};

// Bits of sHdSample::spuAttr.
enum {
    kSpuAttrDryL = 0x01,
    kSpuAttrDryR = 0x02,
    kSpuAttrEffectL = 0x04,
    kSpuAttrEffectR = 0x08,
    kSpuAttrCore0 = 0x10,
    kSpuAttrCore1 = 0x20,
    kSpuAttrPreserved = 0x40, // Undetermined; remix and channel effects retain it.
};

// Bit of the xflags argument of _fire_off_sample().
enum {
    kFireChorusVoice = 0x01,
};

// Error returns of _fire_off_sample() and hs_note_on().
enum {
    kFireBadSample = -1,
    kFireNoSlot = -2,
};
enum {
    kNoteOnBadChannel = -1,
    kNoteOnBadBank = -2,
    kNoteOnNoBank = -3,
    kNoteOnBadProgram = -4,
    kNoteOnNoBankAgain = -5,
    kNoteOnNoProgram = -6,
    kNoteOnNoSplit = -7,
    kNoteOnNoSampleSet = -8,
    kNoteOnNoSample = -9,
};

// Sentinels of the bank records.
enum {
    kHdNoOffset = -1,
    kHdNoIndex = 0xffff,
};

// Volume slides.
enum {
    kSpuVolSweep = 0x8000, // A volume register with this bit set stores a sweep rate.
    kSpuVolSweepDown = 0x2000,
    kSttTypeInstant = 5,
    kSttTypeStep = 6,
    kSttSweepMidpoint = 2048,
    kSttSweepFloor = 256,
    kSttTargetFrames = 15,
    kLinTimeFrames = 30, // A lin_time entry is the volume a sweep rate covers in a frame pair.
    kLinTimeFramePair = 2,
};

// MIDI status bytes, with the channel in the low nibble.
enum {
    kMidiStatusMask = 0xf0,
    kMidiChannelMask = 0x0f,
    kMidiNoteOff = 0x80,
    kMidiNoteOn = 0x90,
    kMidiPolyPressure = 0xa0,
    kMidiControlChange = 0xb0,
    kMidiProgramChange = 0xc0,
    kMidiChannelPressure = 0xd0,
    kMidiPitchBend = 0xe0,
};

// Controller numbers HandleMidiMessage() acts on.
enum {
    kMidiControllerBankSelect = 0,
    kMidiControllerModulation = 1,
    kMidiControllerVolume = 7,
    kMidiControllerPan = 10,
    kMidiControllerExpression = 11,
    kMidiControllerBankSelectLow = 32,
    kMidiControllerSmoothVolume = 80,
    kMidiControllerChorus = 81,
    kMidiControllerCore0Effect = 82,
    kMidiControllerCore1Effect = 83,
    kMidiControllerPriority = 88,
    kMidiControllerKeepPan = 89,
    kMidiControllerResetAll = 121,
    kMidiControllerAllNotesOff = 123,
};

enum {
    kMidiDataBits = 7,
    kMidiSwitchOn = 64, // Smallest switch controller value that means on.
    kMidiShortMessage = 2,
    kMidiMessage = 3,
    kPriorityLevels = 3, // Priority controller values below this select a level.
    kPriorityLevelStep = 48,
    kPriorityLevelBase = 16,
    kAllChannelMask = (1 << HSYN_CHANNELS) - 1,
};

enum {
    kMidiBufferSize = 1024, // Bytes of one MIDI stream buffer, header included.
    kMidiInputBuffers = 2,  // Stream buffers the EE writes to.
};

enum {
    kSynthTickMicroseconds = 2083,
    kChorusRateFractionBits = 16,
    kMicrosecondsPerMillisecond = 1000,
    kInitialChorusDepth = 1024,
    kTickThreadPriority = 29,
    kTickThreadStackSize = 2048,
};

enum {
    kShowSynthStateAll = 0xff,
    kHardSynthInfoShowState = 0,
    kHardSynthInfoShowStateAlt = 1, // Prints the same report as kHardSynthInfoShowState.
    kHardSynthInfoToggleMono = 2,
};

// NTSC-U/C: 0x6e80, PAL: 0x6e80
static int gContextSet = 0;
// NTSC-U/C: 0x6e84, PAL: 0x6e84
static int gXtraDbg = 0;
// NTSC-U/C: 0x6e88, PAL: 0x6e88
static int gSynthFrame = 0;
// NTSC-U/C: 0x6e8c, PAL: 0x6e8c
static int gClockUse = 0;
// NTSC-U/C: 0x6e90, PAL: 0x6e90
static int gPauseCount = 0;
// NTSC-U/C: 0x6e94, PAL: 0x6e94
static int gChansChanged = 0;
// NTSC-U/C: 0x6e98, PAL: 0x6e98
static sSynthConfig gSynthConfig = {
    .stt_speed = 1024,
    .stt_limit = 6144,
    .stt_type = 2,
    .chorus_rate = {200, 300},
    .chorus_depth = {30, 20},
    .rnd_nopause_channels = 0x8000,
};
// NTSC-U/C: 0x6ebc, PAL: 0x6ecc
static sSynthRun gSynthRun;
// NTSC-U/C: 0x6ec4, PAL: 0x6ed4
unsigned char *gpHd = NULL;
// NTSC-U/C: 0x6ec8, PAL: 0x6ed8
unsigned char *gpBd = NULL;
// NTSC-U/C: 0x6ed4, PAL: 0x6ee4
static int move_step = 512;
// NTSC-U/C: 0x6ed8, PAL: 0x6ee8
static unsigned short smooth_bits[] = {8, 17, 27, 31, 37, 0};
// NTSC-U/C: 0x6ee4, PAL: 0x6ef4
static int stt_type = kSttTypeStep;
// NTSC-U/C: 0x6ee8, PAL: 0x6ef8
static int stt_intercept = 1;
// NTSC-U/C: 0x6eec, PAL: 0x6efc
static unsigned short vol_read_reg[] = {SD_VP_VOLXL, SD_VP_VOLXR};
// NTSC-U/C: 0x6ef0, PAL: 0x6f00
static unsigned int lin_time[] = {
    655320, 546100, 468085, 364066, 327660, 273050, 218440, 182033, 156028, 136525, 112986, 91016,
    79917,  68262,  56493,  44884,  39477,  33779,  27305,  21844,  19274,  17245,  14246,  11298,
    9929,   8401,   7123,   5649,   4964,   4255,   3523,   2730,   2520,   2184,   1724,   1424,
    1213,   1056,   885,    712,    618,    528,    442,    352,    297,    273,    218,    172,
    156,    131,    109,    88,     78,     65,     55,     44,     38,     33,     27,
};
// NTSC-U/C: 0x6fdc, PAL: 0x6fec
int rate_L = 192;
// NTSC-U/C: 0x6fe0, PAL: 0x6ff0
int rate_R = 352;
// NTSC-U/C: 0x6ff0, PAL: 0x7000
int gReg_VMixR[HSYN_CORES];
// NTSC-U/C: 0x6ff8, PAL: 0x7008
int gVMixEL[HSYN_CORES];
// NTSC-U/C: 0x7000, PAL: 0x7010
int voice_alloc[HSYN_CORES];
// NTSC-U/C: 0x7008, PAL: 0x7018
int gVMixR[HSYN_CORES];
// NTSC-U/C: 0x7010, PAL: 0x7020
int gReg_VMixEL[HSYN_CORES];
// NTSC-U/C: 0x7018, PAL: 0x7028
int gVMixER[HSYN_CORES];
// NTSC-U/C: 0x7020, PAL: 0x7030
int gVMixL[HSYN_CORES];
// NTSC-U/C: 0x7028, PAL: 0x7038
int gReg_VMixER[HSYN_CORES];
// NTSC-U/C: 0x7030, PAL: 0x7040
int gEndX[HSYN_CORES];
// NTSC-U/C: 0x7038, PAL: 0x7048
int gReg_VMixL[HSYN_CORES];
// NTSC-U/C: 0x7040, PAL: 0x7050
static char hsBf_T[kMidiInputBuffers * kMidiBufferSize];
// NTSC-U/C: 0x7840, PAL: 0x7850
static char hsBf_R[kMidiBufferSize];
// NTSC-U/C: 0x7c40, PAL: 0x7c50
static int gReg_kon[HSYN_CORES];
// NTSC-U/C: 0x7c48, PAL: 0x7c58
static int gReg_koff[HSYN_CORES];
// NTSC-U/C: 0x7c50, PAL: 0x7c60
static SysClock gClock;
// NTSC-U/C: 0x7c58, PAL: 0x7c68
static int pan_2_vol[kPanPositions][2];
// NTSC-U/C: 0x8058, PAL: 0x8068
static int slot_2_mask[kSlotCodes];
// NTSC-U/C: 0x8158, PAL: 0x8168
static short chr_curve[kChorusCurveLength];
// NTSC-U/C: 0x8558, PAL: 0x8568
static TimerCtx gTimer;
// NTSC-U/C: 0x8564, PAL: 0x8574
static int gThid;
// NTSC-U/C: 0x85f0, PAL: 0x8600
unsigned char *gaHds[HSYN_BANKS];
// NTSC-U/C: 0x8630, PAL: 0x8640
sSynNote gCurrentNotes[HSYN_NOTES];
// NTSC-U/C: 0x8e00, PAL: 0x8e10
sSynChannel gChan[HSYN_CHANNELS];
// NTSC-U/C: 0x8f00, PAL: 0x8f10
unsigned char *gaBds[HSYN_BANKS];

// A bank header chunk. The header stores each chunk's position as a byte offset.
static inline sHdChunk *HdChunk(int offset) {
    return (sHdChunk *)&gpHd[offset];
}

// A record of a bank header chunk. The chunk stores each record's position as a byte offset.
static inline void *HdRecord(sHdChunk *pChunk, int index) {
    return &((unsigned char *)pChunk)[pChunk->offsets[index]];
}

// One MIDI stream buffer of hsBf_T.
static inline sceCslMidiStream *InputStream(int buf) {
    return (sceCslMidiStream *)&hsBf_T[buf * kMidiBufferSize];
}

// The stream buffer the tick parses from.
static inline sceCslMidiStream *ParseStream(void) {
    return (sceCslMidiStream *)hsBf_R;
}

// NTSC-U/C: 0x0470, PAL: 0x0470
static void _build_voice2mask(void) {
    int core;
    int voice;

    for (core = 0; core < HSYN_CORES; ++core) {
        for (voice = 0; voice < HSYN_VOICES_PER_CORE; ++voice) {
            slot_2_mask[SD_VOICE(voice) | core] = 1 << voice;
        }
    }
}

// NTSC-U/C: 0x054c, PAL: 0x054c
static void _build_pantable(void) {
    int i;

    for (i = 0; i < kPanPositions / 2; ++i) {
        pan_2_vol[kPanPositions - 1 - i][1] = pan_2_vol[i][0] = kPanFullGain - i * kPanNearStep;
        pan_2_vol[kPanPositions - 1 - i][0] = pan_2_vol[i][1] = i * kPanFarStep;
    }
}

void _build_chorus(int iDepth) {
    int i;

    (void)iDepth; // The binary never reads the depth.
    for (i = 0; i <= kChorusQuarterWave; ++i) {
        chr_curve[i] = (i * kChorusPeak) / kChorusQuarterWave;
    }
    for (; i <= 2 * kChorusQuarterWave; ++i) {
        chr_curve[i] = chr_curve[2 * kChorusQuarterWave - i];
    }
    for (; i <= kChorusThreeQuarterWave; ++i) {
        chr_curve[i] = -chr_curve[i - 2 * kChorusQuarterWave];
    }
    for (; i < kChorusCurveLength; ++i) {
        chr_curve[i] = chr_curve[2 * kChorusThreeQuarterWave - i];
    }
}

int HandleTransIntr(int ch, void *common) {
    int *c = common;

    (void)ch;
    ++*c;
    return 1;
}

int MemCpy_IOPtoSPU(void *pIOP, void *pSPU, int iBlockSize) {
    sceSdVoiceTrans(
        SD_CORE_0, SD_TRANS_MODE_WRITE | SD_TRANS_BY_DMA, pIOP, (unsigned int)pSPU, iBlockSize);
    while (!sceSdVoiceTransStatus(SD_CORE_0, SD_TRANS_STATUS_CHECK)) {
    }
    return 0;
}

int StartAutoDMA(void *pIOP, void *pSPU, int iDirection) {
    (void)pIOP;
    (void)pSPU;
    (void)iDirection;
    return -1;
}

int StopAutoDMA(void *pIOP, void *pSPU, int iDirection) {
    (void)pIOP;
    (void)pSPU;
    (void)iDirection;
    return -1;
}

void _init_channels(void) {
    int i;

    for (i = 0; i < HSYN_CHANNELS; ++i) {
        gChan[i].iVol = kDefaultChannelVolume;
        gChan[i].iPan = kDefaultChannelPan;
        gChan[i].iBank = 0;
        gChan[i].iMod = 0;
        gChan[i].iExp = kDefaultChannelExpression;
        gChan[i].iProg = 0;
        gChan[i].iPitch = 0; // Zero, not the centre of the bend range.
        gChan[i].iBankHi = kDefaultChannelBankHi;
        gChan[i].iFx = 0;
        gChan[i].iPri = kDefaultChannelPriority;
    }
}

void _init_banks(void) {
    int i;

    for (i = 0; i < HSYN_BANKS; ++i) {
        gaHds[i] = gaBds[i] = NULL;
    }
}

void hs_prog_change(int iChan, int iProg) {
    gChan[iChan].iProg = iProg;
}

int get_free_slot(int which_core) {
    static int last_voice[HSYN_CORES];
    int core;
    int voice;

    for (core = 0; core < HSYN_CORES; ++core) {
        if (which_core != kAnyCore && which_core != core) {
            continue;
        }
        // The search stops before last_voice itself. The last voice handed out is not handed out
        // again straight away, even when it is the only free voice.
        for (voice = (last_voice[core] + 1) % HSYN_VOICES_PER_CORE; voice != last_voice[core];
             voice = (voice + 1) % HSYN_VOICES_PER_CORE) {
            if (!(voice_alloc[core] & slot_2_mask[SD_VOICE(voice)])) {
                last_voice[core] = voice;
                return SD_VOICE(voice) | core;
            }
        }
    }
    return kNoSlot;
}

// NTSC-U/C: 0x0e24, PAL: 0x0e24
static int _find_note(int iChan, int iNote) {
    int i;

    for (i = 0; i < HSYN_NOTES; ++i) {
        if ((gCurrentNotes[i].flag & (HSYN_NOTE_ON | HSYN_NOTE_RELEASED)) == HSYN_NOTE_ON &&
            gCurrentNotes[i].chan == iChan && gCurrentNotes[i].note == iNote) {
            return i;
        }
    }
    return kNoNote;
}

// NTSC-U/C: 0x0f48, PAL: 0x0f48
static int _free_note(sSynNote *pNote, int in_play) {
    (void)in_play;
    if (!(pNote->flag & HSYN_NOTE_ON)) {
        return -1;
    }
    pNote->flag = 0;
    return 0;
}

// NTSC-U/C: 0x0fc4, PAL: 0x0fc4
static sSynNote *_new_note(void) {
    int i;

    for (i = 0; i < HSYN_NOTES; ++i) {
        if (!(gCurrentNotes[i].flag & HSYN_NOTE_ON)) {
            return &gCurrentNotes[i];
        }
    }
    return NULL;
}

// NTSC-U/C: 0x1094, PAL: 0x1094
static int _count_notes(void) {
    int i;
    int cnt = 0;

    for (i = 0; i < HSYN_NOTES; ++i) {
        if (gCurrentNotes[i].flag & HSYN_NOTE_ON) {
            ++cnt;
        }
    }
    return cnt;
}

// NTSC-U/C: 0x114c, PAL: 0x114c
static int _search_for_slot(int which_core, int new_pri, int vol) {
    int slot = get_free_slot(which_core);
    int choice = kNoNote;
    int i;

    (void)vol;
    if (slot == kNoSlot) {
        char min = kMaxNotePriority;
        char pri[HSYN_NOTES];

        for (i = 0; i < HSYN_NOTES; ++i) {
            pri[i] = kMaxNotePriority;
            if (!(gCurrentNotes[i].flag & HSYN_NOTE_ON)) {
                continue;
            }
            if (which_core != kAnyCore && which_core != (gCurrentNotes[i].slot & kSlotCoreMask)) {
                continue;
            }
            pri[i] = gCurrentNotes[i].pri;
            if (gCurrentNotes[i].flag & HSYN_NOTE_RELEASED) {
                if (pri[i] > kReleasedPriorityPenalty) {
                    pri[i] = pri[i] - kReleasedPriorityPenalty;
                } else {
                    pri[i] = 0;
                }
            }
            if (pri[i] < kStealPriority) {
                choice = i;
                // The binary does not set min on this path and compares a stale stack value.
                // Taking the chosen priority lets the low-priority note be stolen as intended.
                min = pri[i];
                break;
            }
        }
        if (choice == kNoNote) {
            for (i = 0; i < HSYN_NOTES; ++i) {
                if (pri[i] < min) {
                    choice = i;
                    min = pri[i];
                }
            }
        }
        if (choice != kNoNote) {
            new_pri = kMaxNotePriority; // The caller's priority is discarded once a note is chosen.
        }
        if (choice != kNoNote && min < new_pri) {
            slot = gCurrentNotes[choice].slot;
            voice_alloc[slot & kSlotCoreMask] &= ~slot_2_mask[slot];
            _free_note(&gCurrentNotes[choice], 1);
        }
    }
    return slot;
}

// NTSC-U/C: 0x1550, PAL: 0x1550
static void hs_tick_setup(void) {
    int i;

    for (i = 0; i < HSYN_CORES; ++i) {
        gEndX[i] = sceSdGetSwitch(i | SD_S_ENDX);
        gVMixL[i] = sceSdGetSwitch(i | SD_S_VMIXL);
        gVMixEL[i] = sceSdGetSwitch(i | SD_S_VMIXEL);
        gVMixR[i] = sceSdGetSwitch(i | SD_S_VMIXR);
        gVMixER[i] = sceSdGetSwitch(i | SD_S_VMIXER);
        gReg_kon[i] = 0;
        gReg_koff[i] = 0;
        gReg_VMixL[i] = gVMixL[i];
        gReg_VMixEL[i] = gVMixEL[i];
        gReg_VMixR[i] = gVMixR[i];
        gReg_VMixER[i] = gVMixER[i];
    }
    GetSystemTime(&gClock);
    gClockUse = gClock.low >> kClockUseShift;
    ++gSynthFrame;
}

// NTSC-U/C: 0x1824, PAL: 0x1824
static void do_kOff(int slot) {
    gReg_koff[slot & kSlotCoreMask] |= slot_2_mask[slot];
    if (gReg_kon[slot & kSlotCoreMask] & slot_2_mask[slot]) {
        gReg_kon[slot & kSlotCoreMask] &= ~slot_2_mask[slot];
    }
}

// NTSC-U/C: 0x1964, PAL: 0x1964
static void do_kOn(int slot) {
    gReg_kon[slot & kSlotCoreMask] |= slot_2_mask[slot];
    if (gReg_koff[slot & kSlotCoreMask] & slot_2_mask[slot]) {
        gReg_koff[slot & kSlotCoreMask] &= ~slot_2_mask[slot];
    }
}

// NTSC-U/C: 0x1aa4, PAL: 0x1aa4
static void CheckEffBits(int iSet, int *effArr, int iSlot, int reg) {
    int mask = slot_2_mask[iSlot];
    int cureff = effArr[iSlot & kSlotCoreMask] & mask;

    (void)reg; // The binary takes the switch register and never reads it.
    if ((!iSet && cureff) || (iSet && !cureff)) {
        if (!iSet) {
            effArr[iSlot & kSlotCoreMask] &= ~mask;
        } else {
            effArr[iSlot & kSlotCoreMask] |= mask;
        }
    }
}

int _note_2_pitch(int base_note, int new_note, int detune, int samp_rate) {
    int scePitch = sceSdNote2Pitch(base_note, 0, new_note, detune);

    scePitch = (scePitch * samp_rate) / kSpuSampleRate;
    if (scePitch > kMaxPitch) {
        scePitch = kMaxPitch;
    }
    return scePitch;
}

int _apply_channel_to_note(sSynNote *pNote, int do_set) {
    int i_vols[2];
    int pan;
    unsigned short vol;
    unsigned short use_pitch;
    unsigned short i_pitch;

    pan = pNote->i_pan;
    i_vols[0] = pNote->t_vol[0];
    i_vols[1] = pNote->t_vol[1];
    i_pitch = pNote->c_pitch;
    vol = pNote->i_vol;
    vol = vol * gChan[pNote->chan].iVol / kMidiFullScale;
    vol = vol * gChan[pNote->chan].iExp / kMidiFullScale;
    if (pNote->flag & HSYN_NOTE_NEW) {
        pNote->c_pitch = pNote->i_pitch;
    }
    if (gPauseCount > 0 && !((gSynthConfig.rnd_nopause_channels >> pNote->chan) & 1)) {
        use_pitch = 0;
    } else {
        use_pitch = pNote->c_pitch;
    }
    if (gSynthRun.mono_mode || (gSynthRun.remix_mode && !(pNote->fx & HSYN_FX_KEEP_PAN))) {
        pan = kMidiPanCentre;
    }
    if (!gSynthRun.mono_mode && (pNote->fx & HSYN_FX_CHORUS)) {
        // The chorus pair splits hard left and right, and pan is not read again.
        if (pNote->slave == HSYN_SLAVE_VOICE) {
            pan = kMidiPanRight;
            pNote->t_vol[0] = 0;
            pNote->t_vol[1] = vol * pan_2_vol[kMidiPanCentre][1] / kQ15One;
        } else {
            pan = kMidiPanLeft;
            pNote->t_vol[0] = vol * pan_2_vol[kMidiPanCentre][0] / kQ15One;
            pNote->t_vol[1] = 0;
        }
        (void)pan;
    } else {
        pNote->t_vol[0] = vol * pan_2_vol[pan][0] / kQ15One;
        pNote->t_vol[1] = vol * pan_2_vol[pan][1] / kQ15One;
    }
    if (pNote->flag & HSYN_NOTE_NEW) {
        pNote->c_vol[0] = pNote->t_vol[0];
        pNote->c_vol[1] = pNote->t_vol[1];
    }
    if (do_set) {
        sceSdSetParam(SD_VP_VOLL | pNote->slot, pNote->c_vol[0]);
        sceSdSetParam(SD_VP_VOLR | pNote->slot, pNote->c_vol[1]);
        sceSdSetParam(SD_VP_PITCH | pNote->slot, use_pitch);
    }
    return (pNote->c_pitch != i_pitch || pNote->t_vol[0] != i_vols[0] ||
            pNote->t_vol[1] != i_vols[1]);
}

int _fire_off_sample(int iSamp,
                     int iNote,
                     int iChan,
                     int iVol,
                     sHdProgram *pProgOffs,
                     sHdSplit *pSplitOffs,
                     int xflags) {
    int vagIdx;
    int vagAddr;
    int adsr1;
    int adsr2;
    int pitch_val;
    int transpose;
    int detune;
    int pan;
    int pri;
    int flag;
    int slot;
    int needed_core;
    unsigned short samp_rate;
    unsigned short vol;
    sSynNote *pNote;
    unsigned char spuAttr;
    sHdSample *pSampOffs;
    sHdVagInfo *pVagOffs;
    sHdChunk *pSamp;
    sHdChunk *pVag;

    transpose = 0;
    detune = 0;
    pan = kMidiPanCentre;
    flag = HSYN_NOTE_ON | HSYN_NOTE_NEW;
    pSamp = HdChunk(((sHdHeader *)gpHd)->sampleChunk);
    pVag = HdChunk(((sHdHeader *)gpHd)->vagInfoChunk);
    pSampOffs = HdRecord(pSamp, iSamp); // Formed before the index is checked, as in the binary.
    if (pSamp->maxIndex < iSamp) {
        return kFireBadSample;
    }
    vagIdx = pSampOffs->vagIndex;
    spuAttr = pSampOffs->spuAttr;
    if (gSynthRun.remix_mode) {
        spuAttr = (spuAttr & kSpuAttrPreserved) | kSpuAttrDryL | kSpuAttrDryR;
    }
    vol = pSampOffs->volume * iVol;
    vol = vol * pProgOffs->volume / kMidiFullScale;
    if (pSplitOffs) {
        transpose += pSplitOffs->transpose;
        detune += pSplitOffs->detune;
        vol = vol * pSplitOffs->volume / kMidiFullScale;
        pan = pSplitOffs->pan;
    }
    pri = gChan[iChan].iPri; // The binary first sets kDefaultVoicePriority and discards it.
    pVagOffs = HdRecord(pVag, vagIdx);
    vagAddr = (int)(gpBd + pVagOffs->offset); // gpBd is the SPU2 address of the bank body.
    if (pVagOffs->attribute == HD_VAG_LOOPED) {
        flag |= HSYN_NOTE_LOOPED;
    }
    samp_rate = pVagOffs->sampleRate;
    transpose += pProgOffs->transpose;
    detune += pProgOffs->detune;
    if (pProgOffs->pan != kMidiPanCentre) {
        pan = pProgOffs->pan;
    }
    if (pSampOffs->pan != kMidiPanCentre) {
        pan = pSampOffs->pan;
    }
    adsr1 = pSampOffs->adsr1;
    adsr2 = pSampOffs->adsr2;
    pitch_val = _note_2_pitch(pSampOffs->baseNote, iNote + transpose, detune, samp_rate);
    if (gChan[iChan].iFx & HSYN_CHAN_FX_CORE0_EFFECT) {
        spuAttr = (spuAttr & (kSpuAttrPreserved | kSpuAttrDryL | kSpuAttrDryR)) | kSpuAttrCore0 |
                  kSpuAttrEffectL | kSpuAttrEffectR;
    } else if (gChan[iChan].iFx & HSYN_CHAN_FX_CORE1_EFFECT) {
        spuAttr = (spuAttr & (kSpuAttrPreserved | kSpuAttrDryL | kSpuAttrDryR)) | kSpuAttrCore1 |
                  kSpuAttrEffectL | kSpuAttrEffectR;
    }
    if (gChan[iChan].iFx & HSYN_CHAN_FX_CHORUS) {
        spuAttr |= kSpuAttrDryL | kSpuAttrDryR;
    }
    if (spuAttr & kSpuAttrCore1) {
        needed_core = SD_CORE_1;
    } else if (spuAttr & kSpuAttrCore0) {
        needed_core = SD_CORE_0;
    } else {
        needed_core = kAnyCore;
    }
    slot = _search_for_slot(needed_core, pri, vol);
    if (slot == kNoSlot) {
        return kFireNoSlot;
    }
    pNote = _new_note();
    pNote->slot = slot;
    pNote->chan = iChan;
    pNote->note = iNote;
    pNote->flag = flag;
    pNote->slave = (xflags & kFireChorusVoice) ? HSYN_SLAVE_VOICE : 0;
    pNote->i_pitch = pitch_val;
    pNote->i_vol = vol;
    pNote->i_pan = pan;
    pNote->pri = gChan[iChan].iPri - 1;
    pNote->chr_idx = 0;
    pNote->stt_frame[1] = 0;
    pNote->stt_frame[0] = 0;
    pNote->fx = 0;
    pNote->bank = gChan[iChan].iBank;
    if (gChan[iChan].iFx & HSYN_CHAN_FX_CHORUS) {
        int c_range = (xflags & kFireChorusVoice) ? gSynthConfig.chorus_depth[1]
                                                  : gSynthConfig.chorus_depth[0];
        int chorus_pitch = _note_2_pitch(
            pSampOffs->baseNote, iNote + transpose, detune + c_range, samp_rate);

        pNote->fx |= HSYN_FX_CHORUS;
        pNote->chr_rng = chorus_pitch - pitch_val;
    }
    if (gChan[iChan].iFx & HSYN_CHAN_FX_KEEP_PAN) {
        pNote->fx |= HSYN_FX_KEEP_PAN;
    }
    _apply_channel_to_note(pNote, 1);
    sceSdSetParam(SD_VP_ADSR1 | slot, adsr1);
    sceSdSetParam(SD_VP_ADSR2 | slot, adsr2);
    sceSdSetAddr(SD_VA_SSA | slot, vagAddr);
    CheckEffBits(spuAttr & kSpuAttrDryL, gReg_VMixL, slot, SD_S_VMIXL);
    CheckEffBits(spuAttr & kSpuAttrDryR, gReg_VMixR, slot, SD_S_VMIXR);
    CheckEffBits(spuAttr & kSpuAttrEffectL, gReg_VMixEL, slot, SD_S_VMIXEL);
    CheckEffBits(spuAttr & kSpuAttrEffectR, gReg_VMixER, slot, SD_S_VMIXER);
    do_kOn(slot);
    voice_alloc[slot & kSlotCoreMask] |= slot_2_mask[slot];
    return pNote - gCurrentNotes;
}

int hs_note_on(int iChan, int iNote, int iVol) {
    int i;
    int splitcnt;
    int iOffs;
    int sample;
    int note1;
    int note2;
    sHdProgram *pProgOffs;
    sHdSplit *pSplitOffs = NULL;
    sHdChunk *pProg;
    sHdChunk *pSet;
    sHdSampleSet *pSetOffs;

    if (iChan < 0 || iChan >= HSYN_CHANNELS) {
        return kNoteOnBadChannel;
    }
    if (gChan[iChan].iBank > HSYN_BANKS) { // Yes, bank 16 passes and indexes past gaHds.
        return kNoteOnBadBank;
    }
    gpHd = gaHds[gChan[iChan].iBank];
    gpBd = gaBds[gChan[iChan].iBank];
    if (!gpHd || !gpBd) {
        return kNoteOnNoBank;
    }
    pProg = HdChunk(((sHdHeader *)gpHd)->programChunk);
    pSet = HdChunk(((sHdHeader *)gpHd)->sampleSetChunk);
    if (pProg->maxIndex < gChan[iChan].iProg) {
        return kNoteOnBadProgram;
    }
    if (!gpHd || !gpBd) { // The binary repeats the bank check.
        return kNoteOnNoBankAgain;
    }
    iOffs = pProg->offsets[gChan[iChan].iProg];
    if (iOffs == kHdNoOffset) {
        return kNoteOnNoProgram;
    }
    pProgOffs = HdRecord(pProg, gChan[iChan].iProg);
    splitcnt = pProgOffs->splitCount;
    for (i = 0; i < splitcnt; ++i) {
        if (iNote >= pProgOffs->split[i].rangeLow && pProgOffs->split[i].rangeHigh >= iNote) {
            pSplitOffs = &pProgOffs->split[i];
            break;
        }
    }
    if (!pSplitOffs) {
        return kNoteOnNoSplit;
    }
    if (pSplitOffs->sampleSetIndex == kHdNoIndex) {
        return kNoteOnNoSampleSet;
    }
    pSetOffs = HdRecord(pSet, pSplitOffs->sampleSetIndex);
    sample = pSetOffs->sampleIndex[0];
    if (sample == -1) { // Never true, because the index is read unsigned.
        return kNoteOnNoSample;
    }
    note1 = _fire_off_sample(sample, iNote, iChan, iVol, pProgOffs, pSplitOffs, 0);
    if (note1 >= 0 && (gChan[iChan].iFx & HSYN_CHAN_FX_CHORUS)) {
        note2 = _fire_off_sample(
            sample, iNote, iChan, iVol, pProgOffs, pSplitOffs, kFireChorusVoice);
        gCurrentNotes[note1].slave = note2; // A failed second voice stores its error code.
    }
    return note1;
}

int hs_kill_idx(sSynNote *pNote, int iAlreadyOff) {
    int slot = pNote->slot;

    if (!iAlreadyOff && !(pNote->flag & HSYN_NOTE_RELEASED)) {
        do_kOff(slot);
        pNote->flag |= HSYN_NOTE_RELEASED;
    }
    sceSdSetParam(SD_VP_VOLL | slot, 0);
    sceSdSetParam(SD_VP_VOLR | slot, 0);
    voice_alloc[slot & kSlotCoreMask] &= ~slot_2_mask[slot];
    return _free_note(pNote, 0);
}

int hs_idx_off(int noteidx) {
    int slot;

    if (!(gCurrentNotes[noteidx].flag & HSYN_NOTE_ON)) {
        return -1;
    }
    slot = gCurrentNotes[noteidx].slot;
    do_kOff(slot);
    gCurrentNotes[noteidx].flag |= HSYN_NOTE_RELEASED;
    return 0;
}

int hs_note_off(int iChan, int iNote) {
    int noteidx = _find_note(iChan, iNote);

    if (noteidx == kNoNote) {
        return -1;
    }
    hs_idx_off(noteidx);
    if (gCurrentNotes[noteidx].slave != 0 && gCurrentNotes[noteidx].slave != HSYN_SLAVE_VOICE) {
        hs_idx_off(gCurrentNotes[noteidx].slave);
        gCurrentNotes[noteidx].slave = 0;
        // The link was cleared on the line above. The assignment clears note 0's link.
        gCurrentNotes[gCurrentNotes[noteidx].slave].slave = 0;
    }
    return 0;
}

int hs_check_playing(sSynNote *pNote) {
    int slot = pNote->slot;

    if (pNote->flag & HSYN_NOTE_NEW) {
        pNote->flag &= ~HSYN_NOTE_NEW;
    } else {
        int checkEnv = 0;

        if (pNote->flag & HSYN_NOTE_RELEASED) {
            checkEnv = 1;
        } else if (!(pNote->flag & HSYN_NOTE_LOOPED) &&
                   (gEndX[slot & kSlotCoreMask] & slot_2_mask[slot])) {
            checkEnv = 1;
        }
        if (checkEnv && sceSdGetParam(SD_VP_ENVX | slot) == 0) {
            pNote->flag |= HSYN_NOTE_RELEASED;
            hs_kill_idx(pNote, 1);
            return -1;
        }
    }
    return 0;
}

// NTSC-U/C: 0x37fc, PAL: 0x37fc
static int _pick_lin_val(int target_delta, unsigned char *frames) {
    int lo = 0;
    int hi = (sizeof(lin_time) / sizeof(lin_time[0])) - 1;
    int mid;

    // The two end cases divide by one lin_time entry, the search result by a frame pair.
    if (lin_time[lo] * kLinTimeFrames < (unsigned int)target_delta) {
        *frames = target_delta / lin_time[lo];
        return lo;
    }
    if ((unsigned int)target_delta < lin_time[hi] * kLinTimeFrames) {
        *frames = target_delta / lin_time[hi];
        return hi;
    }
    do {
        mid = (lo + hi) / 2;
        if (lin_time[mid] * kLinTimeFrames < (unsigned int)target_delta) {
            hi = mid;
        } else {
            lo = mid;
        }
    } while (lo + 1 < hi);
    *frames = target_delta / (lin_time[hi] * kLinTimeFramePair);
    return hi;
}

// NTSC-U/C: 0x3a70, PAL: 0x3a70
static int _move_vol_towards(sSynNote *pNote, int which) {
    int delta = 0;
    int resamp = 0;
    int distance;

    if (!(gChan[pNote->chan].iFx & HSYN_CHAN_FX_SMOOTH_VOL)) {
        if (pNote->c_vol[which] == pNote->t_vol[which]) {
            return 0;
        }
        pNote->c_vol[which] = pNote->t_vol[which];
        return 1;
    }
    if (pNote->stt_frame[which] != 0) {
        if (--pNote->stt_frame[which] == 0) {
            resamp = 1;
            pNote->t_vol[which] = pNote->stt_targ[which];
        }
    }
    if (!(pNote->t_vol[which] & kSpuVolSweep) && !(pNote->c_vol[which] & kSpuVolSweep)) {
        delta = pNote->t_vol[which] - pNote->c_vol[which];
    } else if (pNote->t_vol[which] == pNote->c_vol[which]) {
        delta = 0;
    } else if (!(pNote->t_vol[which] & kSpuVolSweep)) {
        resamp = 1;
    } else {
        resamp = 2;
    }
    if (resamp) {
        pNote->c_vol[which] = sceSdGetParam(pNote->slot | vol_read_reg[which]);
        if (resamp == 2) {
            pNote->t_vol[which] = pNote->c_vol[which];
        }
        delta = pNote->t_vol[which] - pNote->c_vol[which];
    }
    if (delta == 0) {
        return 0;
    }
    distance = (delta < 0) ? -delta : delta;
    if (stt_type == kSttTypeInstant) {
        pNote->c_vol[which] = pNote->t_vol[which];
    } else if (stt_type != kSttTypeStep && distance > gSynthConfig.stt_limit) {
        int dir;
        int lin_parm;

        if ((pNote->c_vol[which] & (kSpuVolSweep | kSpuVolSweepDown)) ==
            (kSpuVolSweep | kSpuVolSweepDown)) {
            dir = 1;
        } else if ((pNote->c_vol[which] & (kSpuVolSweep | kSpuVolSweepDown)) == kSpuVolSweep) {
            dir = -1;
        } else if (pNote->t_vol[which] > kSttSweepMidpoint) {
            dir = 1;
        } else {
            dir = -1;
        }
        if (stt_intercept) {
            if (dir == -1 && pNote->t_vol[which] < kSttSweepFloor) {
                pNote->c_vol[which] = smooth_bits[stt_type] | kSpuVolSweep | kSpuVolSweepDown;
                return 1;
            }
            pNote->stt_frame[which] = kSttTargetFrames;
            pNote->stt_targ[which] = pNote->t_vol[which];
            lin_parm = _pick_lin_val(distance, &pNote->stt_frame[which]);
        } else {
            lin_parm = smooth_bits[gSynthConfig.stt_type];
        }
        if (dir == 1) {
            pNote->c_vol[which] = lin_parm | kSpuVolSweep;
        } else {
            pNote->c_vol[which] = lin_parm | kSpuVolSweep | kSpuVolSweepDown;
        }
        pNote->t_vol[which] = pNote->c_vol[which];
    } else if (distance < move_step) {
        pNote->c_vol[which] = pNote->t_vol[which];
    } else if (delta > 0) {
        pNote->c_vol[which] += move_step;
    } else {
        pNote->c_vol[which] -= move_step;
    }
    return 1;
}

unsigned short _apply_chorus(unsigned short c_pitch,
                             unsigned short rate,
                             unsigned short depth,
                             unsigned short *pos) {
    *pos += rate;
    return c_pitch + chr_curve[*pos >> kChorusPositionShift] * depth / kQ15One;
}

int hs_update_note_and_fx(sSynNote *pNote) {
    int changed = 0;
    int use_pitch;

    if ((gChansChanged >> pNote->chan) & 1) {
        changed += _apply_channel_to_note(pNote, 0);
    }
    if (gPauseCount > 0 && !((gSynthConfig.rnd_nopause_channels >> pNote->chan) & 1)) {
        use_pitch = 0;
    } else if (pNote->fx & HSYN_FX_CHORUS) {
        int rate_chan = (pNote->slave == HSYN_SLAVE_VOICE);

        use_pitch = _apply_chorus(
            pNote->c_pitch, gSynthRun.run_chr_rate[rate_chan], pNote->chr_rng, &pNote->chr_idx);
        ++changed;
    } else {
        use_pitch = pNote->c_pitch;
    }
    changed += _move_vol_towards(pNote, 0);
    changed += _move_vol_towards(pNote, 1);
    if (changed) {
        sceSdSetParam(SD_VP_VOLL | pNote->slot, pNote->c_vol[0]);
        sceSdSetParam(SD_VP_VOLR | pNote->slot, pNote->c_vol[1]);
        sceSdSetParam(SD_VP_PITCH | pNote->slot, use_pitch);
    }
    return 0;
}

int hs_reapply_channel(int iChan) {
    int i;

    for (i = 0; i < HSYN_NOTES; ++i) {
        if ((gCurrentNotes[i].flag & HSYN_NOTE_ON) &&
            (iChan == kAllChannels || gCurrentNotes[i].chan == iChan)) {
            _apply_channel_to_note(&gCurrentNotes[i], 1);
        }
    }
    return 0;
}

int ShowSynthState(int iFlags) {
    (void)iFlags;
    return 0;
}

void ResetSynthState(void) {
    int i;

    for (i = 0; i < HSYN_CHANNELS; ++i) {
        hs_prog_change(i, 0);
    }
    voice_alloc[SD_CORE_1] = 0;
    voice_alloc[SD_CORE_0] = 0;
    _init_channels();
    _init_banks();
}

void _do_reg_out(void) {
    int i;

    for (i = 0; i < HSYN_CORES; ++i) {
        int setinboth = gReg_kon[i] & gReg_koff[i];

        if (setinboth) {
            gReg_koff[i] &= ~setinboth;
        }
        if (gReg_kon[i]) {
            sceSdSetSwitch(i | SD_S_KON, gReg_kon[i]);
        }
        if (gReg_koff[i]) {
            sceSdSetSwitch(i | SD_S_KOFF, gReg_koff[i]);
        }
        if (gReg_VMixL[i] != gVMixL[i]) {
            sceSdSetSwitch(i | SD_S_VMIXL, gReg_VMixL[i]);
        }
        if (gReg_VMixEL[i] != gVMixEL[i]) {
            sceSdSetSwitch(i | SD_S_VMIXEL, gReg_VMixEL[i]);
        }
        if (gReg_VMixR[i] != gVMixR[i]) {
            sceSdSetSwitch(i | SD_S_VMIXR, gReg_VMixR[i]);
        }
        if (gReg_VMixER[i] != gVMixER[i]) {
            sceSdSetSwitch(i | SD_S_VMIXER, gReg_VMixER[i]);
        }
    }
}

void HardSynthAllNotesOff(int iChan, int do_now) {
    int i;

    if (do_now) {
        hs_tick_setup();
    }
    for (i = 0; i < HSYN_NOTES; ++i) {
        if ((gCurrentNotes[i].flag & HSYN_NOTE_ON) &&
            (iChan == kAllChannels || gCurrentNotes[i].chan == iChan)) {
            hs_kill_idx(&gCurrentNotes[i], 0);
        }
    }
    if (iChan == kAllChannels && _count_notes()) {
        ShowSynthState(kShowSynthStateAll);
    }
    if (do_now) {
        _do_reg_out();
    }
}

// Sets or clears a channel effect bit from a switch controller value.
static inline void SetChannelFx(int chan, int value, int bit) {
    if (value >= kMidiSwitchOn) {
        gChan[chan].iFx |= bit;
    } else {
        gChan[chan].iFx &= ~bit;
    }
}

unsigned char *HandleMidiMessage(unsigned char *pMidiMsg) {
    int consumed = kMidiMessage;
    int chan = pMidiMsg[0] & kMidiChannelMask;

    switch (pMidiMsg[0] & kMidiStatusMask) {
    case kMidiNoteOff:
        hs_note_off(chan, pMidiMsg[1]);
        consumed = kMidiShortMessage; // Yes, the binary skips only two bytes of a note off.
        break;
    case kMidiNoteOn:
        hs_note_on(chan, pMidiMsg[1], pMidiMsg[2]);
        break;
    case kMidiPolyPressure:
        break;
    case kMidiControlChange:
        switch (pMidiMsg[1]) {
        case kMidiControllerBankSelect:
            gChan[chan].iBankHi = pMidiMsg[2];
            break;
        case kMidiControllerModulation:
            gChan[chan].iMod = pMidiMsg[2];
            gChansChanged |= 1 << chan;
            break;
        case kMidiControllerVolume:
            gChan[chan].iVol = pMidiMsg[2];
            gChansChanged |= 1 << chan;
            break;
        case kMidiControllerPan:
            gChan[chan].iPan = pMidiMsg[2];
            gChansChanged |= 1 << chan;
            break;
        case kMidiControllerExpression:
            gChan[chan].iExp = pMidiMsg[2];
            gChansChanged |= 1 << chan;
            break;
        case kMidiControllerBankSelectLow:
            gChan[chan].iBank = (gChan[chan].iBankHi << kMidiDataBits) + pMidiMsg[2];
            gChan[chan].iBankHi = 0;
            break;
        case kMidiControllerSmoothVolume:
            SetChannelFx(chan, pMidiMsg[2], HSYN_CHAN_FX_SMOOTH_VOL);
            gChansChanged |= 1 << chan;
            break;
        case kMidiControllerChorus:
            SetChannelFx(chan, pMidiMsg[2], HSYN_CHAN_FX_CHORUS);
            gChansChanged |= 1 << chan;
            break;
        case kMidiControllerCore0Effect:
            SetChannelFx(chan, pMidiMsg[2], HSYN_CHAN_FX_CORE0_EFFECT);
            gChansChanged |= 1 << chan;
            break;
        case kMidiControllerCore1Effect:
            SetChannelFx(chan, pMidiMsg[2], HSYN_CHAN_FX_CORE1_EFFECT);
            gChansChanged |= 1 << chan;
            break;
        case kMidiControllerPriority:
            if (pMidiMsg[2] >= kPriorityLevels) {
                gChan[chan].iPri = pMidiMsg[2];
            } else {
                gChan[chan].iPri = pMidiMsg[2] * kPriorityLevelStep + kPriorityLevelBase;
            }
            break;
        case kMidiControllerKeepPan:
            // Unlike the other switches, the binary does not mark the channel changed here.
            SetChannelFx(chan, pMidiMsg[2], HSYN_CHAN_FX_KEEP_PAN);
            break;
        case kMidiControllerResetAll:
            break;
        case kMidiControllerAllNotesOff:
            HardSynthAllNotesOff(chan, 0);
            break;
        default:
            break;
        }
        break;
    case kMidiProgramChange:
        hs_prog_change(chan, pMidiMsg[1]);
        consumed = kMidiShortMessage;
        break;
    case kMidiChannelPressure:
        consumed = kMidiShortMessage;
        break;
    case kMidiPitchBend:
        // The first data byte is the high seven bits, the reverse of standard MIDI.
        gChan[chan].iPitch = (pMidiMsg[1] << kMidiDataBits) + pMidiMsg[2];
        gChansChanged |= 1 << chan;
        break;
    default:
        break;
    }
    return &pMidiMsg[consumed];
}

int HardSynthLoadBD(int ipBd, int ipSpu, int iSize) {
    return MemCpy_IOPtoSPU((void *)ipBd, (void *)ipSpu, iSize);
}

int HardSynthAttachHDtoBD(int port, int ipHd, int ipSpu, int bank) {
    (void)port;
    if (bank < 0 || bank >= HSYN_BANKS) {
        return -1;
    }
    gaHds[bank] = (unsigned char *)ipHd;
    gaBds[bank] = (unsigned char *)ipSpu; // The body slot records the SPU2 address.
    gContextSet = 1;
    return 0;
}

int HardSynthInvalidateBank(int bank) {
    int i;

    if (!gaHds[bank] || !gaBds[bank]) {
        return -1;
    }
    gaHds[bank] = NULL;
    gaBds[bank] = NULL;
    for (i = 0; i < HSYN_NOTES; ++i) {
        if ((gCurrentNotes[i].flag & HSYN_NOTE_ON) && gCurrentNotes[i].bank == bank) {
            hs_kill_idx(&gCurrentNotes[i], 0);
        }
    }
    return 0;
}

void HardSynthInvalidateHd(unsigned char *pHd) {
    int bank;

    for (bank = 0; bank < HSYN_BANKS; ++bank) {
        if (gaHds[bank] == pHd) {
            HardSynthInvalidateBank(bank);
            break;
        }
    }
}

int HardSynthClearHDBD(void) {
    if (!gContextSet) {
        return -1;
    }
    gContextSet = 0;
    return 0;
}

void MidiBufferSetup(void) {
    int buf;

    ParseStream()->buffsize = kMidiBufferSize;
    ParseStream()->validsize = 0;
    for (buf = 0; buf < kMidiInputBuffers; ++buf) {
        InputStream(buf)->buffsize = kMidiBufferSize;
        InputStream(buf)->validsize = 0;
    }
}

int HardSynthKillOld(void) {
    int i;

    for (i = 0; i < HSYN_NOTES; ++i) {
        if (gCurrentNotes[i].flag & HSYN_NOTE_ON) {
            hs_check_playing(&gCurrentNotes[i]);
        }
    }
    return 0;
}

int HardSynthUpdate(void) {
    int i;

    for (i = 0; i < HSYN_NOTES; ++i) {
        if (gCurrentNotes[i].flag & HSYN_NOTE_ON) {
            hs_update_note_and_fx(&gCurrentNotes[i]);
        }
    }
    gChansChanged = 0;
    return 0;
}

int HardSynthParseNew(unsigned char *pMidiBlock, int iBlockSize, int buf) {
    unsigned char *pBlock = pMidiBlock;

    (void)buf;
    while (pBlock < &pMidiBlock[iBlockSize]) {
        pBlock = HandleMidiMessage(pBlock);
        if (!pBlock) {
            return -1;
        }
    }
    return 0;
}

// NTSC-U/C: 0x5e00, PAL: 0x5e00
static int scan_inbuf(int buf) {
    sceCslMidiStream *pBf = InputStream(buf);
    int size = pBf->validsize;

    if (size > 0) {
        if (gXtraDbg) {
            printf("%d midi bytes\n", size);
        }
        memcpy(ParseStream(), pBf, size + sizeof(sceCslMidiStream));
        pBf->validsize = 0;
        // The binary discards the result.
        HardSynthParseNew(ParseStream()->data, ParseStream()->validsize, buf);
    }
    ParseStream()->validsize = 0;
    return size;
}

// NTSC-U/C: 0x5f00, PAL: 0x5f00
static int hsyn_atick(void) {
    int size;

    while (1) {
        if (gXtraDbg) {
            printf("]");
        }
        SleepThread();
        if (gXtraDbg) {
            printf("[");
        }
        hs_tick_setup();
        HardSynthKillOld();
        size = scan_inbuf(0);
        size += scan_inbuf(1);
        (void)size;
        HardSynthUpdate();
        _do_reg_out();
    }
    return 0;
}

// NTSC-U/C: 0x5fec, PAL: 0x5fec
static unsigned int timer_handler(void *common) {
    TimerCtx *tc = common;

    iWakeupThread(tc->thread_id);
    return tc->count;
}

// NTSC-U/C: 0x6054, PAL: 0x6054
static int make_thread(void) {
    struct ThreadParam param;

    param.attr = TH_C;
    param.entry = (void (*)(void))hsyn_atick;
    param.initPriority = kTickThreadPriority;
    param.stackSize = kTickThreadStackSize;
    param.option = 0;
    return CreateThread(&param);
}

// NTSC-U/C: 0x60c8, PAL: 0x60c8
static int set_timer(TimerCtx *pTimer) {
    SysClock clock;
    int timer_id;

    USec2SysClock(kSynthTickMicroseconds, &clock);
    pTimer->count = clock.low;
    timer_id = AllocHardTimer(TC_SYSCLOCK, TIMER_SIZE_32, TIMER_PRESCALE_1);
    if (timer_id <= 0) {
        return -1;
    }
    pTimer->timer_id = timer_id;
    if (SetTimerHandler(timer_id, pTimer->count, timer_handler, pTimer) != KE_OK) {
        return -2;
    }
    if (SetupHardTimer(timer_id, TC_SYSCLOCK, TM_NO_GATE, TIMER_PRESCALE_1) != KE_OK) {
        return -3;
    }
    return 0;
}

// NTSC-U/C: 0x620c, PAL: 0x620c
static int start_timer(TimerCtx *timer) {
    if (StartHardTimer(timer->timer_id) != KE_OK) {
        return -1;
    }
    return 0;
}

// NTSC-U/C: 0x6288, PAL: 0x6288
static int clear_timer(TimerCtx *timer) {
    if (FreeHardTimer(timer->timer_id) != KE_OK) {
        return -1;
    }
    return 0;
}

// NTSC-U/C: 0x6304, PAL: 0x6304
static int stop_timer(TimerCtx *timer) {
    int ret = StopHardTimer(timer->timer_id);

    if (ret != KE_OK && ret != KE_TIMER_NOT_INUSE) {
        return -1;
    }
    return 0;
}

int HardSynthPause(void) {
    int i;

    if (gPauseCount) {
        return 1;
    }
    gPauseCount = 1;
    for (i = 0; i < HSYN_NOTES; ++i) {
        if ((gCurrentNotes[i].flag & HSYN_NOTE_ON) &&
            !((gSynthConfig.rnd_nopause_channels >> gCurrentNotes[i].chan) & 1)) {
            sceSdSetParam(gCurrentNotes[i].slot | SD_VP_PITCH, 0);
        }
    }
    return 0;
}

int HardSynthResume(void) {
    int i;

    if (gPauseCount <= 0) {
        return -1;
    }
    gPauseCount = 0;
    for (i = 0; i < HSYN_NOTES; ++i) {
        if ((gCurrentNotes[i].flag & HSYN_NOTE_ON) &&
            !((gSynthConfig.rnd_nopause_channels >> gCurrentNotes[i].chan) & 1)) {
            sceSdSetParam(gCurrentNotes[i].slot | SD_VP_PITCH, gCurrentNotes[i].c_pitch);
        }
    }
    return 0;
}

int HardSynthSetRemix(int parm) {
    gSynthRun.remix_mode = parm != 0;
    return 0;
}

int HardSynthSetMono(int parm) {
    gSynthRun.mono_mode = parm != 0;
    gChansChanged = kAllChannelMask;
    return 0;
}

void HardSynthConfig(sSynthConfig *pConfig) {
    int i;

    if (pConfig) {
        memcpy(&gSynthConfig, pConfig, sizeof(sSynthConfig));
    }
    for (i = 0; i < HSYN_CORES; ++i) {
        // The binary divides signed although chorus_rate is unsigned.
        gSynthRun.run_chr_rate[i] =
            (kSynthTickMicroseconds << kChorusRateFractionBits) /
            ((int)(gSynthConfig.chorus_rate[i] + 1) * kMicrosecondsPerMillisecond);
    }
}

int HardSynthInit(void) {
    _init_channels();
    _init_banks();
    _build_voice2mask();
    _build_pantable();
    _build_chorus(kInitialChorusDepth);
    HardSynthConfig(NULL);
    gThid = make_thread();
    gTimer.thread_id = gThid;
    StartThread(gThid, 0);
    set_timer(&gTimer); // The binary ignores a timer set-up failure.
    HardSynthReset();
    start_timer(&gTimer);
    return (int)hsBf_T; // The EE writes MIDI streams to this IOP address.
}

int HardSynthReset(void) {
    ResetSynthState();
    return 0;
}

int HardSynthShutdown(void) {
    stop_timer(&gTimer);
    clear_timer(&gTimer);
    return 0;
}

void HardSynthInfo(int what) {
    switch (what) {
    case kHardSynthInfoShowState:
    case kHardSynthInfoShowStateAlt:
        ShowSynthState(kShowSynthStateAll);
        break;
    case kHardSynthInfoToggleMono:
        HardSynthSetMono(!gSynthRun.mono_mode);
        break;
    default:
        break;
    }
}
