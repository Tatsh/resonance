#pragma once

#include <cstddef>

class MsgSink;

/**
 * Something that turns one muse into messages over time.
 *
 * `10MusePlayer` in the RTTI descriptor at `0x0086f788`, with no base list. One data word sits
 * ahead of the compiler-generated vptr, which places the vptr at `+0x04` and makes the subobject
 * eight bytes. MultiMusePlayer places it at `+0x30` and is the one implementation recovered; the
 * 0x20-byte NotePlayer that MuseSynth creates for a NoteMsg is the other, and its constructor at
 * `0x001b4328` takes seven arguments.
 *
 * No vtable of this class alone exists in the image, so it is never instantiated. Its table shape
 * comes from MultiMusePlayer's secondary table at `0x007dfd08`, which runs five entries.
 *
 * The three verbs come from MuseSynth, which is the one caller of all three. It calls Start()
 * immediately after creating a player, Stop() on every player it releases, and slot 4 on the
 * player that requests exclusivity.
 */
class MusePlayer {
public:
    /**
     * Allocate a player from the untagged heap.
     *
     * No out-of-line body exists. The destructor's release branch inlines the call.
     *
     * @param nSize The object size the compiler supplies.
     * @return The block.
     */
    void *operator new(size_t nSize);

    /**
     * Release a player to the untagged heap.
     *
     * @param pBlock The block.
     */
    void operator delete(void *pBlock);

    /**
     * @ghidraAddress 0x001aa440
     */
    MusePlayer();

    /**
     * @ghidraAddress 0x001aa3e8
     */
    virtual ~MusePlayer();

    /**
     * Begin playing, sending every message to one sink.
     *
     * Table slot 2. MultiMusePlayer's override at `0x001a9b48` registers the sink, marks itself
     * running, and posts a scheduler command over its muse's timed entries.
     *
     * @param pSink The sink every message goes to.
     */
    virtual void Start(MsgSink *pSink) = 0;

    /**
     * Stop playing.
     *
     * Table slot 3. MultiMusePlayer's override at `0x001aa1b8` performs no work unless it is
     * running, and otherwise releases its own players and withdraws its scheduler command.
     */
    virtual void Stop() = 0;

    /**
     * Table slot 4, verb unrecovered.
     *
     * MuseParent::RetainOnly() performs no work unless this member reports non-zero, so it decides
     * whether a player displaces its siblings. MultiMusePlayer's override at `0x001a9ed0` is one
     * instruction and returns 1. No other implementation is recovered, and one constant body
     * cannot fix the verb.
     *
     * @return Non-zero to displace the sibling players.
     */
    virtual int Slot4() = 0;

private:
    // Serial number, taken from the counter at 0x00686290 that the constructor increments. Nothing
    // recovered so far reads it.
    int mId; // +0x00
};
