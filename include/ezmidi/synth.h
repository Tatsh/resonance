#pragma once

/**
 * Voice and channel state of the EZMIDI synthesiser.
 *
 * All addresses in this header are relative to the EZMIDI image base. The module is an
 * IOP program of its own, so its addresses never coincide with the main program. Globals
 * keep the names the disassembler project uses for them.
 */

/**
 * State of one MIDI channel.
 *
 * Sixteen entries live in `gChan`. The reset values are the MIDI defaults, so the
 * purposes from volume onwards are inferred rather than confirmed.
 */
struct MidiChannel {
    unsigned char mUnknown00;    /**< +0x00. Reset to 0. */
    unsigned char mVolume;       /**< +0x01. Reset to 100. Inferred. */
    unsigned char mPan;          /**< +0x02. Reset to centre. Inferred. */
    unsigned char mExpression;   /**< +0x03. Reset to 127. Inferred. */
    unsigned short mUnknown04;   /**< +0x04. Reset to 0. */
    unsigned short mUnknown06;   /**< +0x06. Reset to 0. */
    unsigned short mUnknown08;   /**< +0x08. Reset to 0. */
    unsigned char mUnknown0A;    /**< +0x0A. Reset to 0xff. */
    unsigned char mUnknown0B;    /**< +0x0B. Reset to 0. */
    unsigned char mUnknown0C;    /**< +0x0C. Reset to 0x40. */
    unsigned char mPadding0D[3]; /**< +0x0D. Untouched by the reset. */
};

/**
 * Bank payload. Contents arrive with the bank loader.
 *
 * Only the address is used so far.
 */
struct BankData {
    unsigned char mReserved00[1];
};

/**
 * Bank header.
 *
 * Holds byte offsets to four offset tables, measured from the header base. Refined
 * when the bank loader lands.
 */
struct BankHeader {
    unsigned char mReserved00[0x24];
    int mProgTab0;   /**< +0x24. Program table offset. Inferred. */
    int mProgTab1;   /**< +0x28. Program table offset. Inferred. */
    int mSampleTab0; /**< +0x2C. Sample table offset. Inferred. */
    int mSampleTab1; /**< +0x30. Sample table offset. Inferred. */
};

/**
 * Offset table into bank data.
 *
 * Entries address descriptors relative to the table base.
 */
struct OffsetTable {
    int mReserved00[3]; /**< +0x00. */
    int mCount;         /**< +0x0C. Inclusive upper bound. */
    int mOffsets[1];    /**< +0x10. */
};

/**
 * One sample descriptor.
 *
 * Addressed through the offset tables. Fields arrive as the voice code uses them.
 */
struct Sample {
    short mUnknown00;              /**< +0x00. Selects the descriptor table entry. */
    unsigned short mUnknown02;     /**< +0x02. */
    unsigned short mUnknown04;     /**< +0x04. */
    unsigned char mUnknown06;      /**< +0x06. */
    unsigned char mReserved07[4];  /**< +0x07. */
    unsigned char mUnknown0B;      /**< +0x0B. */
    unsigned char mReserved0C;     /**< +0x0C. */
    unsigned char mUnknown0D;      /**< +0x0D. */
    unsigned char mReserved0E[2];  /**< +0x0E. */
    unsigned char mUnknown10;      /**< +0x10. */
    unsigned char mReserved11;     /**< +0x11. */
    unsigned short mUnknown12;     /**< +0x12. */
    unsigned short mUnknown14;     /**< +0x14. */
    unsigned char mReserved16[19]; /**< +0x16. */
    unsigned char mUnknown29;      /**< +0x29. */
};

/**
 * One sample descriptor of the second table.
 *
 * May share its layout with `Sample`; kept apart until a use proves it.
 */
struct SampleDesc {
    int mDataOff;              /**< +0x00. Offset into bank data. */
    unsigned short mUnknown04; /**< +0x04. */
    unsigned char mUnknown06;  /**< +0x06. */
    unsigned char mReserved07; /**< +0x07. */
};

/**
 * Note launch parameters from bank program data.
 *
 * Both parameter pointers the voice launcher takes address instances of this shape
 * in different bank regions. Refined with the note-on path.
 */
struct NoteEvent {
    unsigned char mReserved00[6]; /**< +0x00. */
    unsigned char mUnknown06;     /**< +0x06. */
    unsigned char mUnknown07;     /**< +0x07. */
    signed char mUnknown08;       /**< +0x08. */
    signed char mUnknown09;       /**< +0x09. */
    unsigned char mReserved0A[6]; /**< +0x0A. */
    unsigned char mUnknown10;     /**< +0x10. */
    unsigned char mUnknown11;     /**< +0x11. */
    signed char mUnknown12;       /**< +0x12. */
    signed char mUnknown13;       /**< +0x13. */
};

/** Channel states. EZMIDI `0x8e00`. */
extern struct MidiChannel gChan[16];

/** Bank payload per bank. EZMIDI `0x8f00`. */
extern struct BankData *gaBds[16];

/** Current bank header. Set at note-on. EZMIDI `0x6ec4`. Inferred. */
extern struct BankHeader *gpHd;

/** Current bank data. Set at note-on. EZMIDI `0x6ec8`. Inferred. */
extern struct BankData *gpBd;

/** Bank header per bank. EZMIDI `0x85f0`. */
extern struct BankHeader *gaHds[16];

/**
 * One stereo gain pair of the pan table.
 *
 * 128 entries live in `pan_2_vol`. The builder writes them forward and mirrored.
 */
struct PanPair {
    int mLeft;  /**< +0x00. Inferred. */
    int mRight; /**< +0x04. Inferred. */
};

/** Pan gain pairs. EZMIDI `0x7c58`. */
extern struct PanPair pan_2_vol[128];

/** Chorus modulation curve. 512 entries at EZMIDI `0x8158`. */
extern short chr_curve[512];

/**
 * Reset every channel to its default state.
 *
 * EZMIDI `0x9c4`.
 */
void _init_channels(void);

/**
 * Forget every bank payload and header.
 *
 * EZMIDI `0xb4c`.
 */
void _init_banks(void);

/**
 * Fill the pan gain pairs, forward and mirrored.
 *
 * EZMIDI `0x54c`.
 */
void _build_pantable(void);

/**
 * Fill the chorus modulation curve.
 *
 * Takes a size the binary never reads. EZMIDI `0x668`.
 *
 * @param nDepth Unused size in bytes.
 */
void _build_chorus(int nDepth);

/**
 * Test effect bits with a mode.
 *
 * With a zero mode, clears the masked bits when any are set. With a nonzero mode, sets
 * them when none are set. EZMIDI `0x1aa4`.
 *
 * @param nMode Zero clears, nonzero sets. Inferred.
 * @param pWords Word pair to adjust. Inferred.
 * @param nIndex Mask table index.
 * @param nUnused Ignored by the binary.
 */
void CheckEffBits(int nMode, unsigned int *pWords, int nIndex, int nUnused);

/**
 * Convert a note to an SPU pitch register value.
 *
 * EZMIDI `0x1c44`.
 *
 * @param nNote Note value. Inferred.
 * @param nFine Fine tune. Inferred.
 * @param nTune Tune. Inferred.
 * @param nScale Pitch scale. Inferred.
 * @return The pitch, clamped to 0x3fff.
 */
int _note_2_pitch(int nNote, int nFine, int nTune, int nScale);

/**
 * One playing note.
 *
 * Fifty entries live in `gCurrentNotes`. Fields arrive as the voice code uses them;
 * purposes marked inferred come from a single use each.
 */
struct Note {
    unsigned char mChannel;       /**< +0x00. Indexes `gChan`. Inferred. */
    unsigned char mUnknown01;     /**< +0x01. Key in `_find_note`. */
    unsigned char mUnknown02;     /**< +0x02. Filter bit and voice bits. */
    unsigned char mUnknown03;     /**< +0x03. Compared against 0xff. */
    unsigned char mFlags;         /**< +0x04. Bit 0 marks use; bits 2 and 3 gate updates. */
    unsigned char mUnknown05;     /**< +0x05. Bit 0 and bit 2 steer gains. */
    unsigned short mUnknown06;    /**< +0x06. Scaled by volume. */
    unsigned short mUnknown08;    /**< +0x08. Copied to `mUnknown0C`. */
    unsigned char mPan;           /**< +0x0A. Indexes `pan_2_vol`. Inferred. */
    unsigned char mPadding0B;     /**< +0x0B. */
    unsigned short mUnknown0C;    /**< +0x0C. */
    unsigned short mUnknown0E;    /**< +0x0E. */
    unsigned short mUnknown10;    /**< +0x10. */
    unsigned short mUnknown12;    /**< +0x12. */
    unsigned short mUnknown14;    /**< +0x14. */
    unsigned short mUnknown16;    /**< +0x16. */
    unsigned short mUnknown18;    /**< +0x18. Pitch difference. Inferred. */
    unsigned char mReserved1A[4]; /**< +0x1A. */
    unsigned char mUnknown1E;     /**< +0x1E. */
    unsigned char mUnknown1F;     /**< +0x1F. */
    unsigned char mUnknown20;     /**< +0x20. Count weight in `_search_for_slot`. */
    unsigned char mUnknown21;     /**< +0x21. */
    unsigned char mReserved22[6]; /**< +0x22. */
};

/**
 * One voice allocation mask.
 *
 * The table is read two ways: one entry per voice, and flat words per group. Both
 * shapes below are observed and neither invents a byte.
 */
union SlotMasks {
    struct {
        int mMask;      /**< +0x00. */
        int mUnknown04; /**< +0x04. */
    } mEntries[24];
    int mWords[48]; /**< Flat view. */
};

/** Voice allocation words. EZMIDI `0x7000`. */
extern unsigned int voice_alloc[48];

/** Last voice per group. EZMIDI `0x6ecc`. */
extern int last_voice[2];

/** Voice masks. EZMIDI `0x8058`. */
extern union SlotMasks slot_2_mask;

/** Playing notes. EZMIDI `0x8630`. */
extern struct Note gCurrentNotes[50];

/** Key-on voice shadows. EZMIDI `0x7c40`. */
extern unsigned int gReg_kon[2];

/** Key-off voice shadows. EZMIDI `0x7c48`. */
extern unsigned int gReg_koff[2];

/** Remix flag, set by `HardSynthSetRemix`. EZMIDI `0x6ec0`. Inferred. */
extern unsigned char gRemixMode;

/** Mono flag, set by `HardSynthSetMono`. EZMIDI `0x6ec1`. Inferred. */
extern unsigned char gMonoMode;

/** Pause counter. EZMIDI `0x6e90`. */
extern int gPauseCount;

/** Per-channel run bits. EZMIDI `0x6ebc`. */
extern unsigned short gSynthRun;

/** Fixed gain for one branch. EZMIDI `0x7e58`. Inferred. */
extern int gFixedGainA;

/** Fixed gain for the other branch. EZMIDI `0x7e5c`. Inferred. */
extern int gFixedGainB;

/** Alternate tune for one mode. EZMIDI `0x6ea8`. Inferred. */
extern int gTuneAlt0;

/** Alternate tune for the other mode. EZMIDI `0x6eac`. Inferred. */
extern int gTuneAlt1;

/** Voice mix right shadows. EZMIDI `0x6ff0`. */
extern unsigned int gReg_VMixR[2];

/** Voice mix effect-left shadows. EZMIDI `0x7010`. */
extern unsigned int gReg_VMixEL[2];

/** Voice mix effect-right shadows. EZMIDI `0x7028`. */
extern unsigned int gReg_VMixER[2];

/**
 * Key off a voice group.
 *
 * Sets the group's bits in the key-off shadow and clears them in the key-on shadow.
 * EZMIDI `0x1824`.
 *
 * @param nGroup Voice group. Inferred.
 */
void do_kOff(int nGroup);

/**
 * Key on a voice group.
 *
 * Sets the group's bits in the key-on shadow and clears them in the key-off shadow.
 * EZMIDI `0x1964`.
 *
 * @param nGroup Voice group. Inferred.
 */
void do_kOn(int nGroup);

/**
 * Take a voice slot.
 *
 * EZMIDI `0xc2c`.
 *
 * @param nGroup Voice group, or -1 for either group.
 * @return The slot, or -1 when both groups are full.
 */
int get_free_slot(int nGroup);

/**
 * Fire a voice for a sample.
 *
 * Assembles voice parameters from the bank tables, takes a note, programs the SPU
 * registers, and returns the note index. EZMIDI `0x2240`.
 *
 * @param nSample Sample index. Inferred.
 * @param nChannel Channel index. Inferred.
 * @param nKey Key value. Inferred.
 * @param nScale Pitch scale. Inferred.
 * @param pEvent Note event parameters. Inferred.
 * @param pExtra Extra note event parameters. Inferred.
 * @param nMode Alternate voice mode. Inferred.
 * @return The note index, or -2 without a slot.
 */
int _fire_off_sample(int nSample,
                     int nChannel,
                     int nKey,
                     int nScale,
                     const struct NoteEvent *pEvent,
                     const struct NoteEvent *pExtra,
                     int nMode);

/**
 * Recompute a note's voice gains.
 *
 * Scales the note level through its channel, picks pan or fixed gains, optionally
 * writes the voice registers, and reports whether the cached gains moved.
 * EZMIDI `0x1d14`.
 *
 * @param pNote The note.
 * @param nApply Nonzero writes the voice registers. Inferred.
 * @return Nonzero when the cached gains moved.
 */
int _apply_channel_to_note(struct Note *pNote, int nApply);

/**
 * Find a note by key.
 *
 * EZMIDI `0xe24`.
 *
 * @param nKey0 First key byte. Inferred.
 * @param nKey1 Second key byte. Inferred.
 * @return The note index, or -1.
 */
int _find_note(int nKey0, int nKey1);

/**
 * Release a note.
 *
 * EZMIDI `0xf48`.
 *
 * @param pNote The note.
 * @param nUnknown Ignored by the binary.
 * @return Zero, or -1 when the note is already free.
 */
int _free_note(struct Note *pNote, int nUnknown);

/**
 * Take a free note.
 *
 * EZMIDI `0xfc4`.
 *
 * @return The note, or zero when every note plays.
 */
struct Note *_new_note(void);

/**
 * Count the playing notes.
 *
 * EZMIDI `0x1094`.
 *
 * @return The count.
 */
int _count_notes(void);

/**
 * Take a voice slot for a new note.
 *
 * Falls back to stealing the note with the lowest count when the allocator reports
 * full. EZMIDI `0x114c`.
 *
 * @param nGroup Voice group, or -1 for either group.
 * @param nUnused1 Ignored by the binary.
 * @param nUnused2 Ignored by the binary.
 * @return The slot, or -1.
 */
int _search_for_slot(int nGroup, int nUnused1, int nUnused2);
