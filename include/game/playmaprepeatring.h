#pragma once

#include <vector>

#include "game/playmap.h"

/**
 * Traversal that wraps and repeats a section of the sequence.
 *
 * `PlayMapRepeatRing` in the RTTI descriptor at `0x00901dc0`, with `PlayMap` as its only base at
 * offset 0. Its table runs twenty entries, one past the base's nineteen, and the entry it adds is
 * slot 19. It overrides slots 1, 3 through 6, and 15.
 *
 * The object is 0x48 bytes. One member of its own follows the base at `+0x3c`, a vector of four
 * byte elements holding an ascending run of positions terminated by the literal 10000000. Slots 3,
 * 4, and 15 all operate on it, so its use is recovered even though its purpose is not.
 *
 * That vector and the base's gap vector are read together by slot 15, which indexes the gaps modulo
 * their count. Reading a gap by a wrapped index is what the class name describes, and it is the
 * clearest evidence recovered for what any of these three subclasses does.
 */
class PlayMapRepeatRing : public PlayMap {
public:
    /**
     * Construct a map whose run holds one span from zero to the terminator.
     *
     * Runs the PlayMap constructor, reserves 32 elements in mUnknown3c, and appends 0 and then
     * 10000000, the same state Slot4() resets it to.
     *
     * @ghidraAddress 0x0012b660
     */
    PlayMapRepeatRing();

    /** @ghidraAddress 0x0012d1b8 */
    virtual ~PlayMapRepeatRing();

    /**
     * Stores the position, extends the sequence, and pads the own vector to match.
     *
     * The body calls the base directly rather than through the table, which is what a
     * non-virtual-looking call to a known base compiles to, then dispatches table slot 4 through
     * the vptr so that this class's own reset runs. It then searches mSteps for the position and
     * appends -1 to the own vector once for every element before the match, which leaves the own
     * vector as long as the prefix the position falls at. A miss searches the whole of mSteps and
     * pads by its full length.
     *
     * @param nValue The position to store.
     * @ghidraAddress 0x0012bb60
     */
    virtual void Slot3(int nValue);

    /**
     * Resets the own vector to a single empty span.
     *
     * The body clears the vector and appends 0 and then 10000000, so the class starts from one
     * span covering everything up to that terminator. The zero-length move at the top is the
     * inlined range erase that implements the clear.
     *
     * @ghidraAddress 0x0012ba88
     */
    virtual void Slot4();

    /**
     * Maps the position into one turn of the repeating ring.
     *
     * The span the position falls in selects a section by its index modulo the section count, and
     * the result is that section's first step plus the position's distance into the span, wrapped
     * to the section's length.
     *
     * @param nValue The position to map.
     * @return The mapped position.
     * @ghidraAddress 0x0012d500
     */
    virtual int Slot5(int nValue);

    /**
     * Collects every position between two bounds that plays the same step offset as a position.
     *
     * The walk starts at the span at or before nMin and stops at the first span that starts after
     * nEnd, which the terminator guarantees. Within each span whose wrapped section matches the
     * position's step, it steps by that section's length while below nEnd.
     *
     * @param nStart The first position.
     * @param nMin The lowest position to collect.
     * @param nEnd The position to stop below.
     * @return mUnknown2c.
     * @ghidraAddress 0x0012be68
     */
    virtual std::vector<int> &Slot6(int nStart, int nMin, int nEnd);

    /**
     * Closes the last span and opens a new one.
     *
     * The body overwrites the own vector's last element with the second to last plus nValue times a
     * gap from the base's gap vector, indexed by the second-to-last position modulo the gap count,
     * then appends the 10000000 terminator again. Indexing the gaps by a wrapped index is the
     * repeat in this class's name.
     *
     * @param nValue The multiplier applied to the wrapped gap.
     * @ghidraAddress 0x0012bc48
     */
    virtual void Slot15(int nValue);

    /**
     * Close the open span a bar falls in after the number of passes that reaches the bar.
     *
     * @param nBar The bar.
     * @return 1 when the span after the bar's span was the terminator and has been closed, and 0
     *         otherwise.
     * @ghidraAddress 0x0012bd00
     */
    virtual int Slot16(int nBar);

    /**
     * Reopen the span a bar falls in by making it the last one.
     *
     * The element after the bar's span becomes the terminator and the final element is dropped.
     *
     * @param nBar The bar.
     * @return 0 when the span was already open, and 1 otherwise.
     * @ghidraAddress 0x0012bdd8
     */
    virtual int Slot17(int nBar);

    /**
     * Toggle the span a bar falls in between open and closed.
     *
     * @param nBar The bar.
     * @return 1 when a span was closed, and 0 otherwise.
     * @ghidraAddress 0x0012d5d0
     */
    virtual int Slot18(int nBar);

    /**
     * Append a step with an empty label, through PlayMap::Slot2() called directly.
     *
     * @param nValue The step position.
     * @ghidraAddress 0x0012d4b0
     */
    virtual void Slot19(int nValue);

protected:
    /**
     * Report the index of the span a position falls in.
     *
     * Inline. Slots 16 and 17 expand it, and Slot5() calls the out-of-line copy at `0x0012d588`.
     *
     * @param nValue The position.
     * @return The index of the last element of mUnknown3c at or before the position.
     */
    int SpanIndex(int nValue);

    // Positions in ascending order, terminated by the literal 10000000. Slot 4 resets it to 0 and
    // that terminator, slot 3 pads it with -1, and slot 15 closes its last span. The element type
    // is int from the four-byte stride of every access.
    std::vector<int> mUnknown3c; // +0x3c
};
