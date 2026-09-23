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
 *
 * Only SaveFields() is written here. LoadFields(), PrintFields(), the destructor, and Print() are
 * recorded with their addresses.
 */
class MultiMuse : public Attachment {
public:
    /**
     * Allocate a sequence from the tagged heap under the tag "MultiMuse".
     *
     * No out-of-line body exists. MultiMuseMsg::Load() inlines the call, and it is the one
     * allocation of the class recovered so far.
     *
     * @param nSize The object size the compiler supplies.
     * @return The block.
     */
    void *operator new(size_t nSize);

    /**
     * Release a sequence to the tagged heap.
     *
     * @param pBlock The block.
     */
    void operator delete(void *pBlock);

    /**
     * @ghidraAddress 0x001a8448
     */
    virtual ~MultiMuse();

    /**
     * Write the sequence to a diagnostic stream.
     *
     * Table slot 3. The body is not written. MultiMuseMsg::Print() is the one recovered caller.
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
     * The body is not written. MultiMuseMsg::Load() is the one recovered caller, and it creates a
     * fresh sequence for every read.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x001a8a88
     */
    void LoadFields(IBStream &stream);

    /**
     * Write the sequence to a diagnostic stream without the surrounding braces.
     *
     * The body is not written.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x001a97e8
     */
    void PrintFields(std::ostream &stream);

    /**
     * Schedule a copy of a message at a song position.
     *
     * The sequence stores the message's Clone() rather than the message itself. Both insertion
     * paths keep the entries sorted by position; a non-zero bCheckLast first compares against the
     * last entry and appends when the new position does not precede it, and zero always searches.
     * The body is not written, because neither insertion routine is written yet.
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
     * The body is not written, because the search routine it calls is not written yet.
     *
     * @param nTick The song position, in MIDI ticks.
     * @return The stored message, or null when no entry sits at nTick.
     * @ghidraAddress 0x001a96c8
     */
    MuseMsg *Find(int nTick);

    /**
     * Every message of the sequence, in ascending song position.
     *
     * Public because MultiMusePlayer::Start() reads the start and the finish directly, through a
     * MultiMuse pointer from outside the hierarchy, and the image exposes no accessor. A friend
     * declaration fits equally well. +0x08
     */
    std::vector<TickObj<MuseMsg *> > mEntries;
};
