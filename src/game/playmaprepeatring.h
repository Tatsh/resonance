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
 * byte elements storing an ascending run of span starts terminated by the literal 10000000.
 * SetBarCount(), ResetSpans(), and CloseSpan() all operate on it.
 *
 * CloseSpan() reads the span vector and the base's gap vector together, indexing the gaps modulo
 * their count. Reading a gap by a wrapped index is what the class name describes, and it is
 * the clearest evidence recovered for what any of these three subclasses does.
 */
class PlayMapRepeatRing : public PlayMap {
public:
    /**
     * Construct a map whose run holds one span from zero to the terminator.
     *
     * Runs the PlayMap constructor, reserves 32 elements in mSpanStarts, and appends 0 and then
     * 10000000, the same state ResetSpans() resets it to.
     *
     * @ghidraAddress NTSC-U/C: 0x0012b660
     * @ghidraAddress PAL: 0x0012bd98
     */
    PlayMapRepeatRing();

    /**
     * @ghidraAddress NTSC-U/C: 0x0012d1b8
     * @ghidraAddress PAL: 0x0012d900
     */
    virtual ~PlayMapRepeatRing();

    /**
     * Stores the bar count, resets the spans, and pads the span run to match.
     *
     * The body calls the base directly rather than through the table, which is what a
     * non-virtual-looking call to a known base compiles to, then dispatches ResetSpans() through
     * the vptr, and this class's reset runs. It then searches mSteps for the value and appends -1
     * to mSpanStarts once for every element before the match. The run is then as long as the
     * prefix the value falls at. A miss searches the whole of mSteps and pads by its
     * full length.
     *
     * @param nBarCount The value to store.
     * @ghidraAddress NTSC-U/C: 0x0012bb60
     * @ghidraAddress PAL: 0x0012c2a8
     */
    virtual void SetBarCount(int nBarCount);

    /**
     * Resets mSpanStarts to a single empty span.
     *
     * The body clears the vector and appends 0 and then 10000000, so the class starts from one
     * span covering everything up to that terminator. The zero-length move at the top is the
     * inlined range erase that implements the clear.
     *
     * @ghidraAddress NTSC-U/C: 0x0012ba88
     * @ghidraAddress PAL: 0x0012c1d0
     */
    virtual void ResetSpans();

    /**
     * Maps the bar into one turn of the repeating ring.
     *
     * The span the bar falls in selects a section by its index modulo the section count, and the
     * result is that section's first step plus the bar's distance into the span, wrapped to the
     * section's length.
     *
     * @param nBar The bar to map.
     * @return The mapped position.
     * @ghidraAddress NTSC-U/C: 0x0012d500
     * @ghidraAddress PAL: 0x0012dc60
     */
    virtual int MapBar(int nBar);

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
     * @return mFoundBars.
     * @ghidraAddress NTSC-U/C: 0x0012be68
     * @ghidraAddress PAL: 0x0012c5b0
     */
    virtual std::vector<int> &FindBarsPlaying(int nStart, int nMin, int nEnd);

    /**
     * Closes the last span and opens a new one.
     *
     * The body overwrites the last element of mSpanStarts with the second to last plus nRepeats
     * times a gap from the base's gap vector, indexed by the second-to-last position modulo the gap
     * count, then appends the 10000000 terminator again. Indexing the gaps by a wrapped index is
     * the repeat in this class's name.
     *
     * @param nRepeats The multiplier applied to the wrapped gap.
     * @ghidraAddress NTSC-U/C: 0x0012bc48
     * @ghidraAddress PAL: 0x0012c390
     */
    virtual void CloseSpan(int nRepeats);

    /**
     * Close the open span a bar falls in after the number of passes that reaches the bar.
     *
     * @param nBar The bar.
     * @return 1 when the span after the bar's span was the terminator and has been closed, and 0
     *         otherwise.
     * @ghidraAddress NTSC-U/C: 0x0012bd00
     * @ghidraAddress PAL: 0x0012c448
     */
    virtual int EndLoop(int nBar);

    /**
     * Reopen the span a bar falls in by making it the last one.
     *
     * The element after the bar's span becomes the terminator and the final element is dropped.
     *
     * @param nBar The bar.
     * @return 0 when the span was already open, and 1 otherwise.
     * @ghidraAddress NTSC-U/C: 0x0012bdd8
     * @ghidraAddress PAL: 0x0012c520
     */
    virtual int StartLoop(int nBar);

    /**
     * Toggle the span a bar falls in between open and closed.
     *
     * @param nBar The bar.
     * @return 1 when a span was closed, and 0 otherwise.
     * @ghidraAddress NTSC-U/C: 0x0012d5d0
     * @ghidraAddress PAL: 0x0012dd30
     */
    virtual int ToggleLoop(int nBar);

    /**
     * Append a step with an empty label, through PlayMap::AddStep() called directly.
     *
     * Slot 19.
     *
     * @param nPosition The step position.
     * @ghidraAddress NTSC-U/C: 0x0012d4b0
     * @ghidraAddress PAL: 0x0012dc10
     */
    virtual void AddUnlabeledStep(int nPosition);

protected:
    /**
     * Report the index of the span a bar falls in.
     *
     * Inline. EndLoop() and StartLoop() expand it, and MapBar() calls the out-of-line copy at
     * `0x0012d588`.
     *
     * @param nBar The bar.
     * @return The index of the last element of mSpanStarts at or before the bar.
     */
    int SpanIndex(int nBar);

    // Span starts in ascending order, terminated by the literal 10000000. ResetSpans() resets it
    // to 0 and that terminator, SetBarCount() pads it with -1, and CloseSpan() closes its last
    // span. The element type is int from the four-byte stride of every access.
    std::vector<int> mSpanStarts; // +0x3c
};
