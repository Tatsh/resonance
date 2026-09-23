#pragma once

class MultiMuse;

/**
 * Copy a sequence with every note moved by a number of semitones.
 *
 * The routine lives in `GsMuseUtil.cpp`, the unit the RTTI of its file-local helper class
 * `_GLOBAL_$N$GsMuseUtil.cpp::Shifter` identifies, apart from MultiMuse's own unit. It reads the
 * sequence only through the public MultiMuse::mEntries, so it is reconstructed as a free function
 * of that utility unit rather than as a MultiMuse member. The title is inferred.
 *
 * A stack Shifter visits every entry. A NoteMsg, and a StdMidiMsg with a note-on or note-off
 * status, is copied with its note number raised by nTrans (wrapping at 256). Any other message
 * whose identity lies in the MuseMsg range is copied unchanged. Each copy is added at the entry's
 * position. The body is not written, because the NoteMsg note byte is private.
 *
 * @param pMuse The sequence to copy.
 * @param nTrans The semitones to add to every note.
 * @return A new sequence with one reference, which the caller releases.
 * @ghidraAddress 0x001ab6d8
 */
MultiMuse *TransposeMuse(MultiMuse *pMuse, int nTrans);
