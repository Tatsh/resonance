#pragma once

#include <cstddef>
#include <vector>

#include "os/hxstr.h"

/**
 * Traversal order over one playable sequence.
 *
 * `PlayMap` in the RTTI descriptor at `0x0086f5b0`, a leaf with no base list. The class is
 * abstract: no data reference to its type function exists anywhere in the image, so no vtable
 * carries it at slot 0 and the class is never instantiated. Three subclasses derive from it,
 * `PlayMapLinear`, `PlayMapRing`, and `PlayMapRepeatRing`, which is what the two pure slots below
 * are for.
 *
 * The table runs to nineteen entries, slots 0 through 18, and terminates on the all-zero entry
 * after them. This class implements every slot except 5 and 6, which are the only two pointing at
 * the shared pure-virtual stub. Most of what it does implement is inert, one slot being a bare
 * return and four returning zero, so the base reads as an interface with defaults rather than as
 * shared behaviour.
 *
 * The subclasses fill those two and then extend the table rather than overriding past its end:
 * `PlayMapRing` adds nothing and keeps nineteen entries, `PlayMapRepeatRing` adds one for twenty,
 * and `PlayMapLinear` adds two for twenty-one. None of the three retains a slot pointing at the
 * pure-virtual stub. All three are concrete.
 *
 * The object is 0x3c bytes with the vptr at `+0x38`. A leaf class with no base places its vptr
 * after its data members under this toolchain, which is why the pointer is last rather than first.
 * Its four vectors are recovered from the destructor at `0x00127158`, which tears them down at
 * `+0x2c`, `+0x1c`, `+0x10`, and `+0x04` and so fixes their declaration order as the reverse.
 * Element widths come from the shift each teardown uses to divide the byte span.
 *
 * The destructor body is empty. Everything the compiled destructor performs is the implicit
 * teardown of those four members, and the release of the object itself sits behind the deleting
 * flag this toolchain passes as a second argument. The declaration is real rather than implicit,
 * because a leaf class with no base acquires a virtual destructor only when one is declared
 * virtual.
 *
 * The class declares its own allocation pair. The release path of the destructor calls the tagged
 * free with the literal `PlayMap` at `0x007d0dd0`, and a tag names the class that declares the
 * operator.
 *
 * Each slot below records its table index. The index is part of the class layout, and the method
 * titles are inferred from the bodies of the base and the three subclasses and from their callers.
 */
class PlayMap {
public:
    /**
     * Allocate a map from the tagged heap under the tag `PlayMap`.
     *
     * Unlike most classes in this tree the operator has an out-of-line body, which forwards the
     * size the compiler supplies and the tag literal to the tagged allocator.
     *
     * @param nSize The object size the compiler supplies.
     * @return The block.
     * @ghidraAddress NTSC-U/C: 0x00127118
     * @ghidraAddress PAL: 0x00127820
     */
    void *operator new(size_t nSize);

    /**
     * Release a map to the tagged heap under the tag `PlayMap`.
     *
     * @param pBlock The block.
     * @ghidraAddress NTSC-U/C: 0x00127138
     * @ghidraAddress PAL: 0x00127840
     */
    void operator delete(void *pBlock);

    /**
     * Construct an empty map whose first step is at zero.
     *
     * Reserves eight elements in mSteps and appends the zero step, then reserves eight in
     * mSectionLengths. Every subclass constructor runs it first.
     *
     * @ghidraAddress NTSC-U/C: 0x001263c0
     * @ghidraAddress PAL: 0x00126a70
     */
    PlayMap();

    /**
     * Inline. PlayMapLinear's destructor expands it, and the address is the out-of-line copy the
     * unit emits for the table.
     *
     * @ghidraAddress NTSC-U/C: 0x00127158
     * @ghidraAddress PAL: 0x00127860
     */
    virtual ~PlayMap() {
    }

    /**
     * Slot 2. Appends one step, its distance from the previous step, and a label.
     *
     * The body appends `nPosition - mSteps.back()` to mSectionLengths, then nPosition to mSteps,
     * then the label to mSectionNames. mSectionLengths therefore stores the gap between consecutive
     * steps and is one element behind mSteps until this call completes. Reading mSteps.back() is
     * unconditional, and the first call requires a step to already be present.
     *
     * The second parameter is passed by value. The body copy-constructs it into mSectionNames
     * through HxStr::HxStr(const HxStr &) at `0x004b7b50` and then releases the parameter's own
     * buffer, which is this toolchain destroying a by-value class parameter in the callee.
     *
     * @param nPosition The step position.
     * @param strLabel The label, passed by value.
     * @ghidraAddress NTSC-U/C: 0x001268b0
     * @ghidraAddress PAL: 0x00126f70
     */
    virtual void AddStep(int nPosition, HxStr strLabel);

    /**
     * Slot 3. Stores its argument in mBarCount and does nothing else.
     *
     * @param nBarCount The value to store.
     * @ghidraAddress NTSC-U/C: 0x00127488
     * @ghidraAddress PAL: 0x00127ba8
     */
    virtual void SetBarCount(int nBarCount);

    /**
     * Slot 4. Empty in this class. PlayMapRepeatRing resets its spans here.
     *
     * @ghidraAddress NTSC-U/C: 0x00127398
     * @ghidraAddress PAL: 0x00127ab8
     */
    virtual void ResetSpans();

    /**
     * Slot 5. Maps a bar to its position in the sequence, pure here and overridden by every
     * subclass.
     *
     * The signature is recovered from both sides. GetPatternIndex() and GetAbsoluteSectionIndex()
     * forward their own argument without touching a1 and then consume the result in v0, and each
     * override reads that argument and returns a value. PlayMapRing at `0x0012e408` returns
     * `(nBar + mBarCount) % mSteps.back()`, the wrap its name implies.
     *
     * GetPatternIndex() and GetAbsoluteSectionIndex() take the delta for this dispatch into a0
     * rather than a1. Using a0 preserves the forwarded argument and is the reason the argument is
     * recoverable at all.
     *
     * @param nBar The bar to map.
     * @return The mapped position.
     */
    virtual int MapBar(int nBar) = 0;

    /**
     * Slot 6. Collects the bars that play one position into mFoundBars, pure here.
     *
     * The signature comes from PlayMapRing at `0x0012dad8`. The override clears mFoundBars, walks
     * a value from the first argument upward in steps of mSteps.back() while it remains below the
     * third, appends every value not below the second, and returns the vector itself. The return is
     * the address of the member, which is a reference in the original.
     *
     * @param nStart The first position.
     * @param nMin The lowest position to collect.
     * @param nEnd The position to stop below.
     * @return mFoundBars.
     */
    virtual std::vector<int> &FindBarsPlaying(int nStart, int nMin, int nEnd) = 0;

    /**
     * Slot 7. Returns its first argument unchanged.
     *
     * PlayMapLinear moves the position to the step linked with its own in a step ring. The second
     * parameter is proven by PlayMapLinear::MapToLinkedStep() indexing its per-set table with it,
     * and by PhraseMgr's routine at `0x001c0298` passing its track there.
     *
     * @param nPosition The position to map.
     * @param nSet The set PlayMapLinear selects its table with. Not read here.
     * @return nPosition.
     * @ghidraAddress NTSC-U/C: 0x001273b8
     * @ghidraAddress PAL: 0x00127ad8
     */
    virtual int MapToLinkedStep(int nPosition, int nSet);

    /**
     * Slot 8. Returns the last element of mSteps, the end of the positions the map has set out.
     *
     * @ghidraAddress NTSC-U/C: 0x001273c0
     * @ghidraAddress PAL: 0x00127ae0
     */
    virtual int GetExtent();

    /**
     * Slot 9. Forwards to GetExtent() through the table rather than calling it directly.
     *
     * Callers read the result as the last bar of the level.
     *
     * @ghidraAddress NTSC-U/C: 0x001273d0
     * @ghidraAddress PAL: 0x00127af0
     */
    virtual int GetEndBar();

    /**
     * Slot 10. Returns the number of sections, one less than the number of elements in mSteps.
     *
     * @ghidraAddress NTSC-U/C: 0x00127440
     * @ghidraAddress PAL: 0x00127b60
     */
    virtual int GetSectionCount();

    /**
     * Slot 11. Returns its argument unchanged.
     *
     * PlayMapLinear returns the section its recorded pattern plays at the index.
     *
     * @param nIndex The index into the pattern.
     * @return nIndex.
     * @ghidraAddress NTSC-U/C: 0x00127458
     * @ghidraAddress PAL: 0x00127b78
     */
    virtual int GetPatternSection(int nIndex);

    /**
     * Slot 12. Reports the index of the step at or before the position MapBar() returns.
     *
     * The body forwards its argument to MapBar() through the table, searches mSteps for the value
     * that returns with an upper bound, and reports the distance from the start to the element
     * before the result. MapBar() is pure here. The search key comes from whichever subclass is
     * running.
     *
     * @param nBar The bar to map through MapBar().
     * @return The step index.
     * @ghidraAddress NTSC-U/C: 0x001276a0
     * @ghidraAddress PAL: 0x00127dc0
     */
    virtual int GetPatternIndex(int nBar);

    /**
     * Slot 13. Reports the same step index with a multiple of the last step folded in.
     *
     * The body performs the search GetPatternIndex() performs, forwarding the same argument to
     * MapBar(), and then adds `nTotal * (nBar - nBar % nTotal)` to the index, where nTotal is
     * mSteps.back(). Multiplying by nTotal after rounding nBar down to a multiple of nTotal scales
     * the term by nTotal twice. The doubling reads as an error, but the binary computes it.
     *
     * @param nBar The bar to map through MapBar() and to fold in.
     * @return The step index plus the folded term.
     * @ghidraAddress NTSC-U/C: 0x00127700
     * @ghidraAddress PAL: 0x00127e20
     */
    virtual int GetAbsoluteSectionIndex(int nBar);

    /**
     * Slot 14. Returns zero, ignoring its argument.
     *
     * The parameter is proven by HudPosition's update at `0x0041b060`, which passes the current
     * bar in a1, and by `PlayMapLinear::IsLooping`, an override that reads it.
     *
     * @param nBar The bar.
     * @return Zero here.
     * @ghidraAddress NTSC-U/C: 0x00127460
     * @ghidraAddress PAL: 0x00127b80
     */
    virtual int IsLooping(int nBar);

    /**
     * Slot 15. Empty in this class, and it takes an argument the empty body cannot reveal.
     *
     * The body is `jr ra` and reads no argument register, which is no evidence about the parameter
     * list. PlayMapRepeatRing's override at `0x0012bc48` multiplies a1 by a step gap before storing
     * the result, so the member takes an int.
     *
     * @param nRepeats The multiplier the override applies to a step gap.
     * @ghidraAddress NTSC-U/C: 0x00127468
     * @ghidraAddress PAL: 0x00127b88
     */
    virtual void CloseSpan(int nRepeats);

    /**
     * Slot 16. Returns zero, ignoring its argument.
     *
     * PlayMapLinear's override at `0x00128ed8` passes a1 to the routine at `0x00129150`, which is
     * what proves the bar parameter. Gamer calls it with the current bar.
     *
     * @param nBar The bar.
     * @ghidraAddress NTSC-U/C: 0x00127470
     * @ghidraAddress PAL: 0x00127b90
     */
    virtual int EndLoop(int nBar);

    /**
     * Slot 17. Returns zero, ignoring its argument.
     *
     * PlayMapRepeatRing's override at `0x0012bdd8` stores a1 as the search key it passes to the
     * routine at `0x00105de8`.
     *
     * @param nBar The bar.
     * @ghidraAddress NTSC-U/C: 0x00127478
     * @ghidraAddress PAL: 0x00127b98
     */
    virtual int StartLoop(int nBar);

    /**
     * Slot 18. Returns zero, ignoring its argument.
     *
     * Both overrides forward a1 to EndLoop() and StartLoop().
     *
     * @param nBar The bar.
     * @ghidraAddress NTSC-U/C: 0x00127480
     * @ghidraAddress PAL: 0x00127ba0
     */
    virtual int ToggleLoop(int nBar);

    /**
     * Report the index of the step at or before a position that has already been mapped.
     *
     * GetPatternIndex() performs the same search on the result of MapBar(). This routine receives
     * the mapped position directly. PhraseDatabase::GetStepValue() is the recovered caller.
     *
     * @param nPosition The mapped position.
     * @return The index of the last step at or before nPosition, or -1 when every step follows it.
     * @ghidraAddress NTSC-U/C: 0x00127490
     * @ghidraAddress PAL: 0x00127bb0
     */
    int FindStepIndex(int nPosition);

    /**
     * Report whether a bar maps exactly onto a step.
     *
     * @param nBar The bar.
     * @return Non-zero when MapBar() of the bar is an element of mSteps. A negative bar reports
     *         zero.
     * @ghidraAddress NTSC-U/C: 0x001274d8
     * @ghidraAddress PAL: 0x00127bf8
     */
    int IsStepStart(int nBar);

    /**
     * Report the bar at which the step containing a bar began.
     *
     * @param nBar The bar.
     * @return nBar less its distance past the last step at or before its mapped position.
     * @ghidraAddress NTSC-U/C: 0x00127548
     * @ghidraAddress PAL: 0x00127c68
     */
    int StepStartBar(int nBar);

    /**
     * Report the bar at which the first step at or after a bar begins.
     *
     * @param nBar The bar.
     * @return nBar plus its distance to the lower bound of its mapped position in mSteps. A
     *         negative bar reports zero.
     * @ghidraAddress NTSC-U/C: 0x001275b0
     * @ghidraAddress PAL: 0x00127cd0
     */
    int NextStepBar(int nBar);

    /**
     * Report the bar at which the first step strictly after a bar begins.
     *
     * @param nBar The bar.
     * @return nBar plus its distance to the upper bound of its mapped position in mSteps. A
     *         negative bar reports zero.
     * @ghidraAddress NTSC-U/C: 0x00127628
     * @ghidraAddress PAL: 0x00127d48
     */
    int FollowingStepBar(int nBar);

    /**
     * The number of bars SetBarCount() stores.
     *
     * Public because BGTrackGraph::BuildSequencer() reads it directly at `0x0013fc7c` as the bound
     * of its walk over the track's bars, and the image has no accessor. PlayMapRing adds it to a
     * position as the offset its ring starts at. +0x00
     */
    int mBarCount;

public:
    /**
     * Ascending sequence of positions.
     *
     * GetExtent(), GetSectionCount(), GetPatternIndex(), and GetAbsoluteSectionIndex() read the
     * last element, the count, and an upper bound over this vector, and AddStep() appends to it.
     * The element type is int, from the four-byte stride of every access. Public because the
     * PhraseDatabase constructor at `0x001b72d8` reads its last element and its count directly and
     * the image exposes no accessor. A friend declaration fits the image equally well. +0x04
     */
    std::vector<int> mSteps;

    /**
     * Length of each section, the gap between one step and the previous one. +0x10
     *
     * AddStep() appends to it. Public because HudPosition's constructor at `0x00419f88` reads it
     * directly, indexed by GetPatternSection(), to size each section of the position display, and
     * the image has no accessor for it.
     */
    std::vector<int> mSectionLengths;

    /**
     * Label of each section, the one AddStep() receives with the step. +0x1c
     *
     * Public because HudPosition's constructor reads it directly, indexed by GetPatternSection(),
     * for the section labels of the position display, and the image has no accessor for it.
     */
    std::vector<HxStr> mSectionNames;

protected:
    // Zeroed by the constructor. No routine of the class or its subclasses reads it.
    int mUnusedValue; // +0x28
    // The bars FindBarsPlaying() collects and returns by reference.
    std::vector<int> mFoundBars; // +0x2c
};
