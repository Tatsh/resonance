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

/** Bank payload. Refined when the bank loader lands. */
struct BankData;

/** Bank header. Refined when the bank loader lands. */
struct BankHeader;

/** Channel states. EZMIDI `0x8e00`. */
extern struct MidiChannel gChan[16];

/** Bank payload per bank. EZMIDI `0x8f00`. */
extern struct BankData *gaBds[16];

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
 * Fifty entries live in `gCurrentNotes`. Only the fields the slot routines touch are
 * known; the rest arrives with the pitch and envelope code.
 */
struct Note {
    unsigned char mUnknown00;      /**< +0x00. Key in `_find_note`. */
    unsigned char mUnknown01;      /**< +0x01. Key in `_find_note`. */
    unsigned char mUnknown02;      /**< +0x02. Filter bit in `_search_for_slot`. */
    unsigned char mPadding03;      /**< +0x03. */
    unsigned char mFlags;          /**< +0x04. Bit 0 marks use; bit 3 excludes matches. */
    unsigned char mReserved05[27]; /**< +0x05. */
    unsigned char mUnknown20;      /**< +0x20. Count weight in `_search_for_slot`. */
    unsigned char mReserved21[15]; /**< +0x21. */
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
extern unsigned int voice_alloc[24];

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
 * Take a free voice slot.
 *
 * EZMIDI `0xc2c`.
 *
 * @param nGroup Voice group, or -1 for either group.
 * @return The slot, or -1 when both groups are full.
 */
int get_free_slot(int nGroup);

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
