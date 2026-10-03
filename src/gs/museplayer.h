#pragma once

#include <cstddef>

class MsgSink;

/**
 * Something that turns one muse into messages over time.
 *
 * Its RTTI descriptor is at `0x0086f788`. It has no base list. One data word sits ahead of the
 * compiler-generated vptr. The vptr is therefore at `+0x04`, and the subobject is eight bytes.
 * MultiMusePlayer places it at `+0x30` and is the one implementation recovered; the 0x20-byte
 * NotePlayer that MuseSynth creates for a NoteMsg is the other, and its constructor at `0x001b4328`
 * takes seven arguments.
 *
 * The class table at `0x007dfe68` runs five entries. Slots 2 through 4 point to the pure-virtual
 * stub at `0x005381a8`, so the class is never instantiated alone. The constructor and destructor
 * install the table while a derived object is being built or torn down.
 *
 * The three verbs come from MuseSynth, which is the one caller of all three. It calls Start()
 * immediately after creating a player, Stop() on every player it releases, and DisplacesSiblings()
 * on the player that requests exclusivity.
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
     * Construct a player with the next serial number.
     *
     * Increments sMuseID and records the new value in mId.
     *
     * @ghidraAddress NTSC-U/C: 0x001aa440
     * @ghidraAddress PAL: 0x001b01a8
     */
    MusePlayer();

    /**
     * Inline. Identical copies sit at `0x001aa3e8` and `0x001aa4a8`, one in each translation unit
     * that emits the table at `0x007dfe68`.
     *
     * @ghidraAddress NTSC-U/C: 0x001aa3e8
     * @ghidraAddress PAL: 0x001b0150
     */
    virtual ~MusePlayer() {
    }

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
     * Report whether this player displaces its siblings.
     *
     * Table slot 4. MuseParent::RetainOnly() does not perform work unless this member reports
     * non-zero.
     * MultiMusePlayer's override at `0x001a9ed0` returns 1 and NotePlayer's at `0x001b41c0`
     * returns zero.
     *
     * @return Non-zero to displace the sibling players.
     */
    virtual int DisplacesSiblings() = 0;

private:
    // Serial number, taken from sMuseID. Nothing recovered so far reads it.
    int mId; // +0x00
};

/**
 * Serial number of the most recently constructed MusePlayer.
 *
 * Starts at 0. MusePlayer's constructor is the one reader and writer.
 *
 * @ghidraAddress NTSC-U/C: 0x00686290
 * @ghidraAddress PAL: 0x006c74f8
 */
extern int sMuseID;
