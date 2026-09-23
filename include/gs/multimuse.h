#pragma once

#include <cstddef>
#include <iostream>
#include <vector>

#include "app/attachment.h"
#include "mid/tickobj.h"
#include "msg/musemsg.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

/**
 * Sequence of messages, each at a song position.
 *
 * `9MultiMuse` in the RTTI descriptor at `0x008ef088`, with Attachment as its one base. The object
 * is 0x14 bytes, which the allocation in MultiMuseMsg::Load() measures: the reference count at
 * `+0x00`, the vptr at `+0x04`, and the vector over `+0x08` through `+0x13`. Its vtable is at
 * `0x007dfb78` and runs four entries, slot 2 retaining Attachment::Destroy().
 *
 * The element type is attested. The one Sequencer instantiation in the image is
 * `Sequencer<TickObj<MuseMsg *> const *>` at `0x008eec48`, and MultiMusePlayer::Start() posts one
 * of those over this vector's start and finish. SaveFields() agrees with that shape independently:
 * it advances eight bytes per element and writes the first word through Mid::MBT::Save() and the
 * second through WriteMessagePointerToStream().
 *
 * MultiMuseMsg carries one of these and is how a sequence moves between a MsgSource and a
 * MsgSink, and MultiMusePlayer is what turns one into messages over time.
 */
class MultiMuse : public Attachment {
public:
    /**
     * Allocate a sequence from the tagged heap under the tag "MultiMuse".
     *
     * MultiMuseMsg::Load() and Phrase inline the call.
     *
     * @param nSize The object size the compiler supplies.
     * @return The block.
     * @ghidraAddress 0x001a9490
     */
    void *operator new(size_t nSize);

    /**
     * Release a sequence to the tagged heap.
     *
     * @param pBlock The block.
     * @ghidraAddress 0x001a94b0
     */
    void operator delete(void *pBlock);

    /**
     * Delete every stored message, then release the vector.
     *
     * @ghidraAddress 0x001a8448
     */
    virtual ~MultiMuse();

    /**
     * Write the sequence to a diagnostic stream.
     *
     * Table slot 3. MultiMuseMsg::Print() is the one recovered caller. An empty sequence prints
     * `[empty]`. Otherwise the entries print through PrintMuseEntry() inside one pair of brackets,
     * one per line. When the stream's buffer is an nlfilebuf, each continuation line is padded with
     * spaces to the column at which the previous entry began.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x001a8580
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write the sequence to an output stream.
     *
     * Writes the entry count as one four-byte transfer and then each entry as a position followed
     * by a message pointer. The loop reloads the vector's start on every iteration and its finish
     * as the loop condition, which is faithful and not a reconstruction artefact.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x001a9738
     */
    void SaveFields(OBStream &stream);

    /**
     * Read the sequence back from an input stream.
     *
     * Empties the sequence, reads the entry count as one four-byte transfer, and reads each entry
     * as a position and a message pointer. MultiMuseMsg::Load() is the one recovered caller, and it
     * creates a fresh sequence for every read.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x001a8a88
     */
    void LoadFields(IBStream &stream);

    /**
     * Schedule a copy of a message at a song position.
     *
     * The sequence stores the message's Clone() rather than the message itself. Both insertion
     * paths keep the entries sorted by position; a non-zero bCheckLast first compares against the
     * last entry and appends when the new position does not precede it, and zero always searches.
     * The two insertion paths are InsertSorted() and InsertAtUpperBound().
     *
     * @param pMsg The message to copy.
     * @param nTick The song position, in MIDI ticks.
     * @param bCheckLast Whether to try appending before searching.
     * @ghidraAddress 0x001a9650
     */
    void Add(MuseMsg *pMsg, int nTick, int bCheckLast);

    /**
     * Find the message scheduled at exactly a song position.
     *
     * The search is std::lower_bound() through TickObjAfter(), called out of line at `0x001a9aa0`.
     *
     * @param nTick The song position, in MIDI ticks.
     * @return The stored message, or null when no entry sits at nTick.
     * @ghidraAddress 0x001a96c8
     */
    MuseMsg *Find(int nTick);

    /**
     * Append a Clone() of every message of another sequence, at the same song positions.
     *
     * mEntries is first grown to hold at least as many entries as the other sequence has. The
     * existing entries are kept and the new ones are appended without sorting. The image has no
     * caller, and the name is inferred.
     *
     * @param other The sequence to copy from.
     * @ghidraAddress 0x001a8808
     */
    void Append(const MultiMuse &other);

    /**
     * Every message of the sequence, in ascending song position.
     *
     * Public because MultiMusePlayer::Start() reads the start and the finish directly, through a
     * MultiMuse pointer from outside the hierarchy, and the image exposes no accessor. A friend
     * declaration fits equally well. +0x08
     */
    std::vector<TickObj<MuseMsg *> > mEntries;
};

/**
 * Write one scheduled message to a diagnostic stream as `[position: message]`.
 *
 * The position arrives by value in a1 and the message in a2, which is how TrackData's bar printer
 * at `0x001d31d8` passes the two halves of one entry. The message is printed through
 * Message::PrintBraced(), whose result is discarded.
 *
 * @param stream The stream to write to.
 * @param position The song position.
 * @param pMsg The message.
 * @return The stream.
 * @ghidraAddress 0x001a97e8
 */
std::ostream &PrintMuseEntry(std::ostream &stream, Mid::MBT position, MuseMsg *pMsg);
