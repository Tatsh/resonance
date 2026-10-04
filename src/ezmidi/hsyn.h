#ifndef EZMIDI_HSYN_H
#define EZMIDI_HSYN_H

#include "ezmidi/ezmidi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The hardware synthesiser of the EZMIDI IOP module, from `midi_hsyn.c`. The layouts and names
 * follow the stabs the shipped EZMIDI.IRX records. Addresses in the comments below are offsets
 * into EZMIDI.IRX.
 */

/** MIDI channels the synthesiser tracks. */
#define HSYN_CHANNELS 16

/** Bank slots of gaHds and gaBds. */
#define HSYN_BANKS 16

/** Notes the synthesiser can have sounding at once. */
#define HSYN_NOTES 50

/** SPU2 cores, each with its own voices. */
#define HSYN_CORES 2

/** Voices of one SPU2 core. */
#define HSYN_VOICES_PER_CORE 24

/** sSynNote::slave value that marks the second voice of a chorus pair. */
#define HSYN_SLAVE_VOICE 255

/** Bits of sSynNote::flag. */
enum {
    HSYN_NOTE_ON = 0x01,       /*!< The note has a voice assigned. */
    HSYN_NOTE_NEW = 0x02,      /*!< The voice was keyed on since the last tick. */
    HSYN_NOTE_LOOPED = 0x04,   /*!< The sample loops. Its end flag is not a release. */
    HSYN_NOTE_RELEASED = 0x08, /*!< The voice has been keyed off. */
};

/** Bits of sSynNote::fx. */
enum {
    HSYN_FX_KEEP_PAN = 0x01, /*!< The note retains its pan in remix mode. */
    HSYN_FX_CHORUS = 0x04,   /*!< The note is one voice of a chorus pair. */
};

/** Bits of sSynChannel::iFx, each switched by one MIDI controller. */
enum {
    HSYN_CHAN_FX_KEEP_PAN = 0x01,     /*!< Controller 89 retains note pan in remix mode. */
    HSYN_CHAN_FX_SMOOTH_VOL = 0x02,   /*!< Controller 80 makes volume changes slide. */
    HSYN_CHAN_FX_CHORUS = 0x04,       /*!< Controller 81 makes each note play a detuned pair. */
    HSYN_CHAN_FX_CORE1_EFFECT = 0x10, /*!< Controller 83 moves notes to core 1 and its effect. */
    HSYN_CHAN_FX_CORE0_EFFECT = 0x20, /*!< Controller 82 moves notes to core 0 and its effect. */
};

/** Per-channel MIDI state. */
typedef struct {
    unsigned char iProg;   /*!< Program. */
    unsigned char iVol;    /*!< Channel volume. */
    unsigned char iPan;    /*!< Pan. */
    unsigned char iExp;    /*!< Expression. */
    unsigned short iBank;  /*!< Bank slot. */
    unsigned short iMod;   /*!< Modulation. */
    unsigned short iPitch; /*!< Pitch bend. */
    unsigned char iBankHi; /*!< Bank select high byte. */
    unsigned char iFx;     /*!< Effect bits, a set of HSYN_CHAN_FX_KEEP_PAN and the others. */
    unsigned char iPri;    /*!< Voice priority. */
    unsigned char iPadc;   /*!< Padding. */
    unsigned short iPads;  /*!< Padding. */
} sSynChannel;

/** One sounding note and the voice that plays it. */
typedef struct {
    unsigned char chan;         /*!< MIDI channel. */
    unsigned char note;         /*!< MIDI note. */
    unsigned char slot;         /*!< Voice, as SD_VOICE() of the voice with the core. */
    unsigned char slave;        /*!< Note of the chorus voice, or #HSYN_SLAVE_VOICE. */
    unsigned char flag;         /*!< State bits, a set of HSYN_NOTE_ON and the others. */
    unsigned char fx;           /*!< Effect bits, a set of HSYN_FX_KEEP_PAN and HSYN_FX_CHORUS. */
    unsigned short i_vol;       /*!< Volume of the note alone. */
    unsigned short i_pitch;     /*!< Pitch register value of the note alone. */
    unsigned char i_pan;        /*!< Pan of the note alone. */
    unsigned char i_pad;        /*!< Padding. */
    unsigned short c_pitch;     /*!< Pitch register value in effect. */
    unsigned short c_vol[2];    /*!< Left and right volume registers in effect. */
    unsigned short t_vol[2];    /*!< Left and right volume registers the note moves towards. */
    unsigned short chr_idx;     /*!< Chorus curve position. */
    unsigned short chr_rng;     /*!< Chorus pitch range. */
    unsigned short stt_targ[2]; /*!< Volumes a timed sweep ends at. */
    unsigned char stt_frame[2]; /*!< Ticks remaining in each timed sweep. */
    unsigned char pri;          /*!< Voice priority. */
    unsigned char bank;         /*!< Bank slot. */
    unsigned char pada[6];      /*!< Padding. */
} sSynNote;

/** Run-time switches the EE sets. */
typedef struct {
    unsigned short run_chr_rate[2]; /*!< Chorus curve step per tick, by pair voice. */
    unsigned char remix_mode;       /*!< Nonzero to centre every note and use the dry mix. */
    unsigned char mono_mode;        /*!< Nonzero for mono output. */
} sSynthRun;

/** The tick timer. */
typedef struct {
    int thread_id; /*!< Thread the timer wakes. */
    int timer_id;  /*!< Hardware timer. */
    int count;     /*!< Compare value in timer cycles. */
} TimerCtx;

/** Bank header. */
typedef struct {
    unsigned char reserved[36]; /*!< +0x00 Version and header chunk fields. */
    int programChunk;           /*!< Byte offset of the program chunk. */
    int sampleSetChunk;         /*!< Byte offset of the sample set chunk. */
    int sampleChunk;            /*!< Byte offset of the sample chunk. */
    int vagInfoChunk;           /*!< Byte offset of the VAG information chunk. */
} sHdHeader;

/** A table chunk of a bank header. */
typedef struct {
    unsigned char reserved[12]; /*!< +0x00 Chunk identifier and size. */
    int maxIndex;               /*!< Highest valid index. */
    int offsets[];              /*!< Byte offset of each record from the chunk, or -1. */
} sHdChunk;

/** A key split of a program. */
typedef struct {
    unsigned short sampleSetIndex; /*!< Sample set the split plays, or 0xFFFF. */
    unsigned char rangeLow;        /*!< Lowest note of the split. */
    unsigned char reserved3;       /*!< +0x03 */
    unsigned char rangeHigh;       /*!< Highest note of the split. */
    unsigned char reserved5[11];   /*!< +0x05 */
    unsigned char volume;          /*!< Volume, 127 for full scale. */
    unsigned char pan;             /*!< Pan, 64 for the centre. */
    signed char transpose;         /*!< Transposition in semitones. */
    signed char detune;            /*!< Fine tuning. */
} sHdSplit;

/** A program record. */
typedef struct {
    unsigned char reserved0[4]; /*!< +0x00 */
    unsigned char splitCount;   /*!< Key splits that follow the record. */
    unsigned char reserved5;    /*!< +0x05 */
    unsigned char volume;       /*!< Volume, 127 for full scale. */
    unsigned char pan;          /*!< Pan, 64 for none. */
    signed char transpose;      /*!< Transposition in semitones. */
    signed char detune;         /*!< Fine tuning. */
    unsigned char reserved[26]; /*!< +0x0A */
    sHdSplit split[];           /*!< Key splits. */
} sHdProgram;

/** A sample set record. */
typedef struct {
    unsigned char reserved[4];    /*!< +0x00 */
    unsigned short sampleIndex[]; /*!< Samples of the set. */
} sHdSampleSet;

/** A sample record. */
typedef struct {
    short vagIndex;               /*!< VAG the sample plays. */
    unsigned char reserved2[9];   /*!< +0x02 */
    unsigned char baseNote;       /*!< Note that plays at the recorded rate. */
    unsigned char reserved12;     /*!< +0x0C */
    unsigned char pan;            /*!< Pan, 64 for none. */
    unsigned char reserved14[2];  /*!< +0x0E */
    unsigned char volume;         /*!< Volume, 127 for full scale. */
    unsigned char reserved17;     /*!< +0x11 */
    unsigned short adsr1;         /*!< First envelope register value. */
    unsigned short adsr2;         /*!< Second envelope register value. */
    unsigned char reserved22[19]; /*!< +0x16 */
    unsigned char spuAttr;        /*!< Output and core bits. */
} sHdSample;

/** A VAG information record. */
typedef struct {
    unsigned int offset;       /*!< Byte offset of the VAG in the bank body. */
    unsigned short sampleRate; /*!< Recorded rate in hertz. */
    unsigned char attribute;   /*!< #HD_VAG_LOOPED when the VAG loops. */
} sHdVagInfo;

/** sHdVagInfo::attribute value of a looping VAG. */
#define HD_VAG_LOOPED 1

/** Bank header of the note being started. */
extern unsigned char *gpHd;

/** SPU2 address of the bank body of the note being started. */
extern unsigned char *gpBd;

/** Bank headers by bank slot. */
extern unsigned char *gaHds[HSYN_BANKS];

/** SPU2 addresses of the bank bodies by bank slot. */
extern unsigned char *gaBds[HSYN_BANKS];

/** Channel state. */
extern sSynChannel gChan[HSYN_CHANNELS];

/** Voices in use, one bit per voice and one word per core. */
extern int voice_alloc[HSYN_CORES];

/** Sounding notes. */
extern sSynNote gCurrentNotes[HSYN_NOTES];

/** Voices whose sample has ended, one word per core, read each tick. */
extern int gEndX[HSYN_CORES];

/** Dry left mix switches read from the SPU2 each tick, one word per core. */
extern int gVMixL[HSYN_CORES];

/** Dry right mix switches read from the SPU2 each tick, one word per core. */
extern int gVMixR[HSYN_CORES];

/** Effect left mix switches read from the SPU2 each tick, one word per core. */
extern int gVMixEL[HSYN_CORES];

/** Effect right mix switches read from the SPU2 each tick, one word per core. */
extern int gVMixER[HSYN_CORES];

/** Dry left mix switches to write at the end of the tick, one word per core. */
extern int gReg_VMixL[HSYN_CORES];

/** Dry right mix switches to write at the end of the tick, one word per core. */
extern int gReg_VMixR[HSYN_CORES];

/** Effect left mix switches to write at the end of the tick, one word per core. */
extern int gReg_VMixEL[HSYN_CORES];

/** Effect right mix switches to write at the end of the tick, one word per core. */
extern int gReg_VMixER[HSYN_CORES];

/** Left chorus rate. The module never reads it. */
extern int rate_L;

/** Right chorus rate. The module never reads it. */
extern int rate_R;

/**
 * Fill the quarter-sine chorus curve.
 *
 * @param iDepth Ignored.
 * @ghidraAddress NTSC-U/C: 0x0668
 * @ghidraAddress PAL: 0x0668
 */
void _build_chorus(int iDepth);

/**
 * Count one completed SPU2 transfer.
 *
 * @param ch Transfer channel.
 * @param common Counter to increment.
 * @return One.
 * @ghidraAddress NTSC-U/C: 0x0868
 * @ghidraAddress PAL: 0x0868
 */
int HandleTransIntr(int ch, void *common);

/**
 * Copy IOP memory to SPU2 memory and wait for the transfer to end.
 *
 * @param pIOP IOP source.
 * @param pSPU SPU2 destination.
 * @param iBlockSize Byte count.
 * @return Zero.
 * @ghidraAddress NTSC-U/C: 0x08c8
 * @ghidraAddress PAL: 0x08c8
 */
int MemCpy_IOPtoSPU(void *pIOP, void *pSPU, int iBlockSize);

/**
 * Start an automatic DMA stream. Not implemented.
 *
 * @param pIOP Ignored.
 * @param pSPU Ignored.
 * @param iDirection Ignored.
 * @return -1.
 * @ghidraAddress NTSC-U/C: 0x0954
 * @ghidraAddress PAL: 0x0954
 */
int StartAutoDMA(void *pIOP, void *pSPU, int iDirection);

/**
 * Stop an automatic DMA stream. Not implemented.
 *
 * @param pIOP Ignored.
 * @param pSPU Ignored.
 * @param iDirection Ignored.
 * @return -1.
 * @ghidraAddress NTSC-U/C: 0x098c
 * @ghidraAddress PAL: 0x098c
 */
int StopAutoDMA(void *pIOP, void *pSPU, int iDirection);

/**
 * Reset every channel to its default MIDI state.
 *
 * @ghidraAddress NTSC-U/C: 0x09c4
 * @ghidraAddress PAL: 0x09c4
 */
void _init_channels(void);

/**
 * Detach every bank slot.
 *
 * @ghidraAddress NTSC-U/C: 0x0b4c
 * @ghidraAddress PAL: 0x0b4c
 */
void _init_banks(void);

/**
 * Select a channel's program.
 *
 * @param iChan Channel.
 * @param iProg Program.
 * @ghidraAddress NTSC-U/C: 0x0be4
 * @ghidraAddress PAL: 0x0be4
 */
void hs_prog_change(int iChan, int iProg);

/**
 * Find a free voice, starting after the one handed out last on each core.
 *
 * @param which_core Core, or -1 for either.
 * @return The voice slot, or -1 when every voice is in use.
 * @ghidraAddress NTSC-U/C: 0x0c2c
 * @ghidraAddress PAL: 0x0c2c
 */
int get_free_slot(int which_core);

/**
 * Convert a note to a pitch register value for a sample.
 *
 * @param base_note Note that plays at the recorded rate.
 * @param new_note Note to play.
 * @param detune Fine tuning.
 * @param samp_rate Recorded rate in hertz.
 * @return The pitch register value, limited to the largest the SPU2 accepts.
 * @ghidraAddress NTSC-U/C: 0x1c44
 * @ghidraAddress PAL: 0x1c44
 */
int _note_2_pitch(int base_note, int new_note, int detune, int samp_rate);

/**
 * Recompute a note's volume and pitch from its channel.
 *
 * @param pNote Note.
 * @param do_set Nonzero to write the registers now.
 * @return Nonzero when the pitch or the target volume changed.
 * @ghidraAddress NTSC-U/C: 0x1d14
 * @ghidraAddress PAL: 0x1d14
 */
int _apply_channel_to_note(sSynNote *pNote, int do_set);

/**
 * Key on one sample of a program on a new voice.
 *
 * @param iSamp Sample index.
 * @param iNote Note.
 * @param iChan Channel.
 * @param iVol Velocity.
 * @param pProgOffs Program record.
 * @param pSplitOffs Key split, or null.
 * @param xflags One for the second voice of a chorus pair.
 * @return The note index, or a negative error code.
 * @ghidraAddress NTSC-U/C: 0x2240
 * @ghidraAddress PAL: 0x2240
 */
int _fire_off_sample(int iSamp,
                     int iNote,
                     int iChan,
                     int iVol,
                     sHdProgram *pProgOffs,
                     sHdSplit *pSplitOffs,
                     int xflags);

/**
 * Start a note on a channel.
 *
 * @param iChan Channel.
 * @param iNote Note.
 * @param iVol Velocity.
 * @return The note index, or a negative error code.
 * @ghidraAddress NTSC-U/C: 0x2c6c
 * @ghidraAddress PAL: 0x2c6c
 */
int hs_note_on(int iChan, int iNote, int iVol);

/**
 * Silence a note at once and release its voice.
 *
 * @param pNote Note.
 * @param iAlreadyOff Nonzero when the voice is already keyed off.
 * @return Zero, or -1 when the note was not sounding.
 * @ghidraAddress NTSC-U/C: 0x3250
 * @ghidraAddress PAL: 0x3250
 */
int hs_kill_idx(sSynNote *pNote, int iAlreadyOff);

/**
 * Key off a note by index.
 *
 * @param noteidx Note index.
 * @return Zero, or -1 when the note was not sounding.
 * @ghidraAddress NTSC-U/C: 0x33cc
 * @ghidraAddress PAL: 0x33cc
 */
int hs_idx_off(int noteidx);

/**
 * Key off the note playing on a channel, with its chorus voice.
 *
 * @param iChan Channel.
 * @param iNote Note.
 * @return Zero, or -1 when the note is not playing.
 * @ghidraAddress NTSC-U/C: 0x34e0
 * @ghidraAddress PAL: 0x34e0
 */
int hs_note_off(int iChan, int iNote);

/**
 * Release a note whose voice has fallen silent.
 *
 * @param pNote Note.
 * @return Zero, or -1 when the note was released.
 * @ghidraAddress NTSC-U/C: 0x3654
 * @ghidraAddress PAL: 0x3654
 */
int hs_check_playing(sSynNote *pNote);

/**
 * Advance a chorus position and apply it to a pitch.
 *
 * @param c_pitch Pitch register value.
 * @param rate Curve step.
 * @param depth Pitch range.
 * @param pos Curve position to advance.
 * @return The pitch register value with the chorus applied.
 * @ghidraAddress NTSC-U/C: 0x4334
 * @ghidraAddress PAL: 0x4334
 */
unsigned short _apply_chorus(unsigned short c_pitch,
                             unsigned short rate,
                             unsigned short depth,
                             unsigned short *pos);

/**
 * Apply one tick of channel changes, chorus, and volume slides to a note.
 *
 * @param pNote Note.
 * @return Zero.
 * @ghidraAddress NTSC-U/C: 0x43f8
 * @ghidraAddress PAL: 0x43f8
 */
int hs_update_note_and_fx(sSynNote *pNote);

/**
 * Recompute and write every note of a channel.
 *
 * @param iChan Channel, or -1 for every channel.
 * @return Zero.
 * @ghidraAddress NTSC-U/C: 0x4654
 * @ghidraAddress PAL: 0x4654
 */
int hs_reapply_channel(int iChan);

/**
 * Print the synthesiser state. Empty in the shipped module.
 *
 * @param iFlags Sections to print.
 * @return Zero.
 * @ghidraAddress NTSC-U/C: 0x477c
 * @ghidraAddress PAL: 0x477c
 */
int ShowSynthState(int iFlags);

/**
 * Reset every program, voice, channel, and bank slot.
 *
 * @ghidraAddress NTSC-U/C: 0x47ac
 * @ghidraAddress PAL: 0x47ac
 */
void ResetSynthState(void);

/**
 * Write the key-on, key-off, and changed voice mix switches of both cores.
 *
 * @ghidraAddress NTSC-U/C: 0x4840
 * @ghidraAddress PAL: 0x4840
 */
void _do_reg_out(void);

/**
 * Silence the notes of a channel.
 *
 * @param iChan Channel, or -1 for every channel.
 * @param do_now Nonzero to read and write the SPU2 switches around the change.
 * @ghidraAddress NTSC-U/C: 0x4bf4
 * @ghidraAddress PAL: 0x4bf4
 */
void HardSynthAllNotesOff(int iChan, int do_now);

/**
 * Act on one MIDI channel message.
 *
 * @param pMidiMsg Message.
 * @return The next message.
 * @ghidraAddress NTSC-U/C: 0x4d70
 * @ghidraAddress PAL: 0x4d70
 */
unsigned char *HandleMidiMessage(unsigned char *pMidiMsg);

/**
 * Copy a bank body to SPU2 memory.
 *
 * @param ipBd IOP address of the body.
 * @param ipSpu SPU2 address.
 * @param iSize Byte count.
 * @return Zero.
 * @ghidraAddress NTSC-U/C: 0x5744
 * @ghidraAddress PAL: 0x5744
 */
int HardSynthLoadBD(int ipBd, int ipSpu, int iSize);

/**
 * Attach a bank header and its body to a bank slot.
 *
 * @param port Ignored.
 * @param ipHd IOP address of the header.
 * @param ipSpu SPU2 address of the body.
 * @param bank Bank slot.
 * @return Zero, or -1 for an invalid slot.
 * @ghidraAddress NTSC-U/C: 0x579c
 * @ghidraAddress PAL: 0x579c
 */
int HardSynthAttachHDtoBD(int port, int ipHd, int ipSpu, int bank);

/**
 * Detach a bank slot and silence its notes.
 *
 * @param bank Bank slot.
 * @return Zero, or -1 when the slot was empty.
 * @ghidraAddress NTSC-U/C: 0x586c
 * @ghidraAddress PAL: 0x586c
 */
int HardSynthInvalidateBank(int bank);

/**
 * Detach the bank slot that uses a header.
 *
 * @param pHd IOP address of the header.
 * @ghidraAddress NTSC-U/C: 0x5a28
 * @ghidraAddress PAL: 0x5a28
 */
void HardSynthInvalidateHd(unsigned char *pHd);

/**
 * Clear the flag an attached bank sets.
 *
 * @return Zero, or -1 when no bank was attached since the last call.
 * @ghidraAddress NTSC-U/C: 0x5acc
 * @ghidraAddress PAL: 0x5acc
 */
int HardSynthClearHDBD(void);

/**
 * Empty the MIDI stream buffers.
 *
 * @ghidraAddress NTSC-U/C: 0x5b20
 * @ghidraAddress PAL: 0x5b20
 */
void MidiBufferSetup(void);

/**
 * Release every note whose voice has fallen silent.
 *
 * @return Zero.
 * @ghidraAddress NTSC-U/C: 0x5b7c
 * @ghidraAddress PAL: 0x5b7c
 */
int HardSynthKillOld(void);

/**
 * Apply one tick of updates to every sounding note.
 *
 * @return Zero.
 * @ghidraAddress NTSC-U/C: 0x5c54
 * @ghidraAddress PAL: 0x5c54
 */
int HardSynthUpdate(void);

/**
 * Act on a block of MIDI messages.
 *
 * @param pMidiBlock Messages.
 * @param iBlockSize Byte count.
 * @param buf Ignored.
 * @return Zero.
 * @ghidraAddress NTSC-U/C: 0x5d34
 * @ghidraAddress PAL: 0x5d34
 */
int HardSynthParseNew(unsigned char *pMidiBlock, int iBlockSize, int buf);

/**
 * Stop the pitch of every note on the channels pausing affects.
 *
 * @return Zero, or one when already paused.
 * @ghidraAddress NTSC-U/C: 0x6390
 * @ghidraAddress PAL: 0x6390
 */
int HardSynthPause(void);

/**
 * Restore the pitch of every note HardSynthPause() stopped.
 *
 * @return Zero, or -1 when not paused.
 * @ghidraAddress NTSC-U/C: 0x64e0
 * @ghidraAddress PAL: 0x64e0
 */
int HardSynthResume(void);

/**
 * Switch remix mode.
 *
 * @param parm Nonzero to enable.
 * @return Zero.
 * @ghidraAddress NTSC-U/C: 0x6660
 * @ghidraAddress PAL: 0x6660
 */
int HardSynthSetRemix(int parm);

/**
 * Switch mono output and recompute every channel.
 *
 * @param parm Nonzero to enable.
 * @return Zero.
 * @ghidraAddress NTSC-U/C: 0x66a4
 * @ghidraAddress PAL: 0x66a4
 */
int HardSynthSetMono(int parm);

/**
 * Replace the settings and recompute the chorus rates.
 *
 * @param pConfig New settings, or null to retain the current settings.
 * @ghidraAddress NTSC-U/C: 0x66f4
 * @ghidraAddress PAL: 0x66f4
 */
void HardSynthConfig(sSynthConfig *pConfig);

/**
 * Initialise the synthesiser, start its tick thread, and start the tick timer.
 *
 * @return The IOP address of the MIDI stream buffers the EE writes to.
 * @ghidraAddress NTSC-U/C: 0x6800
 * @ghidraAddress PAL: 0x6800
 */
int HardSynthInit(void);

/**
 * Reset the synthesiser state.
 *
 * @return Zero.
 * @ghidraAddress NTSC-U/C: 0x68d0
 * @ghidraAddress PAL: 0x68d0
 */
int HardSynthReset(void);

/**
 * Stop and release the tick timer.
 *
 * @return Zero.
 * @ghidraAddress NTSC-U/C: 0x690c
 * @ghidraAddress PAL: 0x690c
 */
int HardSynthShutdown(void);

/**
 * Handle a diagnostic request.
 *
 * @param what Zero or one to print the state, two to switch mono output.
 * @ghidraAddress NTSC-U/C: 0x6960
 * @ghidraAddress PAL: 0x6960
 */
void HardSynthInfo(int what);

#ifdef __cplusplus
}
#endif

#endif
