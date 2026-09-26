#include "ezmidi/synth.h"

// This file mirrors the original note management. Entry order and loop bounds match the
// binary, including the allocator's retry shapes.

// EZMIDI 0x7000
unsigned int voice_alloc[48];

// EZMIDI 0x6ecc
int last_voice[2];

// EZMIDI 0x8058
union SlotMasks slot_2_mask;

// EZMIDI 0x8630
struct Note gCurrentNotes[50];

// EZMIDI 0xc2c
int get_free_slot(int nGroup) {
    int nVoice;

    for (nVoice = 0; nVoice < 2; ++nVoice) {
        int nCandidate;

        if (nGroup != -1 && nGroup != nVoice) {
            continue;
        }
        nCandidate = (last_voice[nVoice] + 1) % 24;
        for (;;) {
            if (nCandidate == last_voice[nVoice]) {
                break;
            }
            if ((voice_alloc[last_voice[nVoice]] & slot_2_mask.mEntries[nCandidate].mMask) == 0) {
                last_voice[nVoice] = nCandidate;
                return (nCandidate << 1) | nVoice;
            }
            nCandidate = (nCandidate + 1) % 24;
        }
    }
    return -1;
}

// EZMIDI 0xe24
int _find_note(int nChannel, int nNote) {
    int nIndex;

    for (nIndex = 0; nIndex < 50; ++nIndex) {
        struct Note *pNote = &gCurrentNotes[nIndex];

        if ((pNote->mFlags & 9) == 1 && pNote->mNote == nNote && pNote->mChannel == nChannel) {
            return nIndex;
        }
    }
    return -1;
}

// EZMIDI 0xf48
int _free_note(struct Note *pNote, int nUnknown) {
    (void)nUnknown; // Yes, the binary takes a second argument and never reads it.

    if ((pNote->mFlags & 1) == 0) {
        return -1;
    }
    pNote->mFlags = 0;
    return 0;
}

// EZMIDI 0xfc4
struct Note *_new_note(void) {
    int nIndex;

    for (nIndex = 0; nIndex < 50; ++nIndex) {
        if ((gCurrentNotes[nIndex].mFlags & 1) == 0) {
            return &gCurrentNotes[nIndex];
        }
    }
    return 0;
}

// EZMIDI 0x1094
int _count_notes(void) {
    int nIndex;
    int nCount = 0;

    for (nIndex = 0; nIndex < 50; ++nIndex) {
        if ((gCurrentNotes[nIndex].mFlags & 1) != 0) {
            ++nCount;
        }
    }
    return nCount;
}

// EZMIDI 0x114c
int _search_for_slot(int nGroup, int nUnused1, int nUnused2) {
    signed char abCounts[50];
    int nSlot = get_free_slot(nGroup);
    int nSelected = -1;
    int nBest = 0x7f;
    int nLimit = 0x7f;
    int nIndex;

    (void)nUnused1; // Yes, the binary takes two more arguments and never reads them.
    (void)nUnused2;

    if (nSlot == -1) {
        for (nIndex = 0; nIndex < 50; ++nIndex) {
            struct Note *pNote;

            abCounts[nIndex] = 0x7f;
            if ((gCurrentNotes[nIndex].mFlags & 1) == 0) {
                continue;
            }
            if (nGroup != -1 && ((gCurrentNotes[nIndex].mUnknown02 & 1) != nGroup)) {
                continue;
            }
            pNote = &gCurrentNotes[nIndex];
            abCounts[nIndex] = (signed char)pNote->mUnknown20;
            if ((pNote->mFlags & 8) != 0) {
                if (abCounts[nIndex] < 11) {
                    abCounts[nIndex] = 0;
                } else {
                    abCounts[nIndex] -= 10;
                }
            }
            if (abCounts[nIndex] >= 10) {
                nSelected = nIndex;
                break;
            }
        }
        // The binary only sets the best mark on the empty path, so it reads whatever the
        // stack holds when the loud-note path jumps over it. A fixed mark keeps the shape.
        if (nSelected == -1) {
            for (nIndex = 0; nIndex < 50; ++nIndex) {
                if (abCounts[nIndex] < nBest) {
                    nSelected = nIndex;
                    nBest = abCounts[nIndex];
                }
            }
        }
        if (nSelected == -1) {
            return -1;
        }
        if (nBest < nLimit) {
            struct Note *pNote = &gCurrentNotes[nSelected];
            int nField = pNote->mUnknown02;
            int nMask;

            nSlot = nField;
            nMask = slot_2_mask.mWords[nField & 1];
            voice_alloc[nField & 1] &= (unsigned int)~nMask;
            _free_note(pNote, 1);
        }
        return nSlot;
    }
    return nSlot;
}
