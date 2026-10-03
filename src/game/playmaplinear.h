#pragma once

#include <vector>

#include "game/playmap.h"

/**
 * Traversal that runs the sequence once from start to end, repeating each section a set number of
 * times.
 *
 * `PlayMapLinear` in the RTTI descriptor at `0x008f2a20`, with `PlayMap` as its only base at
 * offset 0. It overrides the widest set of the three subclasses, slots 1, 5 through 14, and 16
 * through 19, and supplies slot 20, which no other subclass does.
 *
 * The object is 0x74 bytes, which the tagged allocation in the LevelBuilder constructor at
 * `0x001ea8f0` measures. Its own members follow the base at `+0x3c`.
 *
 * The map plays a list of sections, each an index into the base's steps paired with a repeat count.
 * AppendSection() appends one section, and RecordPattern() records the list as the pattern
 * GrowPastLimit() appends again whenever a position runs past the end. A repeat count of
 * kRepeatForever marks a section that loops until EndLoop() ends it at a bar, and StartLoop() sets
 * a section looping again.
 *
 * The unit registers SelfTest() with TestRegistry under the name `PlayMapLinear`.
 */
class PlayMapLinear : public PlayMap {
public:
    /**
     * Construct an empty linear map.
     *
     * Runs the PlayMap constructor, sizes mStepRings to kSetCount empty tables, and reserves eight
     * elements in mWindow and mWindowStarts. A non-zero bLoadStepRings then runs LoadStepRings().
     * LevelBuilder's constructor passes 1, and SelfTest() passes 0.
     *
     * @param bLoadStepRings Whether to fill the partner tables from the script.
     * @ghidraAddress NTSC-U/C: 0x00127a80
     * @ghidraAddress PAL: 0x001281a0
     */
    explicit PlayMapLinear(int bLoadStepRings);

    /**
     * @ghidraAddress NTSC-U/C: 0x0012a4d8
     * @ghidraAddress PAL: 0x0012ac08
     */
    virtual ~PlayMapLinear();

    /**
     * Map a bar to its offset within the base's steps.
     *
     * Finds the window entry the bar falls in and returns that section's first step plus the bar's
     * distance into the entry, wrapped to the section's length.
     *
     * @param nBar The bar to map.
     * @return The mapped position.
     * @ghidraAddress NTSC-U/C: 0x0012ad58
     * @ghidraAddress PAL: 0x0012b490
     */
    virtual int MapBar(int nBar);

    /**
     * Collect every position between two bounds that plays the same step offset as a position.
     *
     * The walk starts at the window entry at or before nMin and runs until an entry starts at or
     * after nEnd. It does not test for the end of the window, and relies on GrowPastLimit() having
     * grown the window past nEnd.
     *
     * @param nStart The position whose section and offset are matched.
     * @param nMin The lowest position to collect.
     * @param nEnd The position to stop below.
     * @return mFoundBars.
     * @ghidraAddress NTSC-U/C: 0x00128d08
     * @ghidraAddress PAL: 0x00129438
     */
    virtual std::vector<int> &FindBarsPlaying(int nStart, int nMin, int nEnd);

    /**
     * Carry a position from its step to the partner step the table for one set records.
     *
     * The position's step comes from PlayMap::FindStepIndex(). Without a record for that step in
     * mStepRings[nSet] the position comes back unchanged.
     *
     * @param nPosition The position to map.
     * @param nSet The index into mStepRings.
     * @return The mapped position.
     * @ghidraAddress NTSC-U/C: 0x0012ae88
     * @ghidraAddress PAL: 0x0012b5c0
     */
    virtual int MapToLinkedStep(int nPosition, int nSet);

    /**
     * Report the position at which the window ends.
     *
     * @return The start of the last window entry plus its section's length times its repeat count,
     *         or zero when the window is empty.
     * @ghidraAddress NTSC-U/C: 0x0012ae38
     * @ghidraAddress PAL: 0x0012b570
     */
    virtual int GetExtent();

    /**
     * @return The window end RecordPattern() recorded.
     * @ghidraAddress NTSC-U/C: 0x0012aa60
     * @ghidraAddress PAL: 0x0012b198
     */
    virtual int GetEndBar();

    /**
     * @return The number of sections in the recorded pattern.
     * @ghidraAddress NTSC-U/C: 0x0012aa68
     * @ghidraAddress PAL: 0x0012b1a0
     */
    virtual int GetSectionCount();

    /**
     * @param nIndex The index into the recorded pattern.
     * @return The section at that index.
     * @ghidraAddress NTSC-U/C: 0x0012aa80
     * @ghidraAddress PAL: 0x0012b1b8
     */
    virtual int GetPatternSection(int nIndex);

    /**
     * Report the index within the recorded pattern of the window entry a bar falls in.
     *
     * @param nBar The bar.
     * @return The window index modulo the pattern length.
     * @ghidraAddress NTSC-U/C: 0x0012af30
     * @ghidraAddress PAL: 0x0012b668
     */
    virtual int GetPatternIndex(int nBar);

    /**
     * Report the absolute index of the window entry a bar falls in.
     *
     * @param nBar The bar.
     * @return The window index plus the count of entries the trim has dropped.
     * @ghidraAddress NTSC-U/C: 0x0012afb8
     * @ghidraAddress PAL: 0x0012b6f0
     */
    virtual int GetAbsoluteSectionIndex(int nBar);

    /**
     * @param nBar The bar.
     * @return 1 when the window entry the bar falls in repeats forever, and 0 otherwise.
     * @ghidraAddress NTSC-U/C: 0x0012b028
     * @ghidraAddress PAL: 0x0012b760
     */
    virtual int IsLooping(int nBar);

    /**
     * End the looping section at a bar.
     *
     * When the window entry the bar falls in repeats forever, its repeat count becomes the number
     * of whole passes up to and including the bar, and every later entry is moved to follow it.
     *
     * @param nBar The bar.
     * @return 1 when the entry was looping, and 0 otherwise.
     * @ghidraAddress NTSC-U/C: 0x00128ed8
     * @ghidraAddress PAL: 0x00129608
     */
    virtual int EndLoop(int nBar);

    /**
     * Set the section a bar falls in looping forever, and move every later entry to follow it.
     *
     * @param nBar The bar.
     * @return Always 1.
     * @ghidraAddress NTSC-U/C: 0x00129038
     * @ghidraAddress PAL: 0x00129768
     */
    virtual int StartLoop(int nBar);

    /**
     * Toggle the loop on the section a bar falls in.
     *
     * @param nBar The bar.
     * @return 1 when a loop was ended, and 0 when one was started.
     * @ghidraAddress NTSC-U/C: 0x0012b0a8
     * @ghidraAddress PAL: 0x0012b7e0
     */
    virtual int ToggleLoop(int nBar);

    /**
     * Record the sections appended so far as the pattern, and the window end with it.
     *
     * Slot 19.
     *
     * @ghidraAddress NTSC-U/C: 0x0012aa18
     * @ghidraAddress PAL: 0x0012b150
     */
    virtual void RecordPattern();

    /**
     * Slot 20. Append one section, played once, at the end of the window.
     *
     * LevelBuilder's constructor calls it once per value of configuration code 0x39d.
     *
     * @param nSection The index of the section's first step.
     * @ghidraAddress NTSC-U/C: 0x00128c38
     * @ghidraAddress PAL: 0x00129368
     */
    virtual void AppendSection(int nSection);

    /**
     * Exercise the map on four steps and seven sections.
     *
     * The routine maps seven positions and toggles two loops, and discards every result. Nothing in
     * the shipped game calls it apart from the test registry.
     *
     * @return Always 1.
     * @ghidraAddress NTSC-U/C: 0x001293b0
     * @ghidraAddress PAL: 0x00129ae0
     */
    static int SelfTest();

    /**
     * Run SelfTest() in the shape TestRegistry::TestFunc requires.
     *
     * The unit's static initialiser registers it. The integer result reaches the `hx.test`
     * command through the return register, which decides its `ok` report.
     *
     * @ghidraAddress NTSC-U/C: 0x0012b110
     * @ghidraAddress PAL: 0x0012b848
     */
    static int RunSelfTest();

protected:
    /** The number of partner tables in mStepRings. */
    static constexpr int kSetCount = 8;

    /** The repeat count that marks a section looping until EndLoop() ends it. */
    static constexpr int kRepeatForever = 10000;

    /** One section of the window, a step index paired with its repeat count. Eight bytes. */
    struct Entry {
        int mSection; /*!< The index into mSteps and mSectionLengths. */
        int mRepeats; /*!< The number of passes, or kRepeatForever. */
    };

    /**
     * One partner record, a step paired with the step MapToLinkedStep() moves it to. Eight bytes.
     *
     * A type distinct from Entry, because the two vectors grow through separate insertion routines
     * (`0x0012a238` for Entry and `0x00129d68` for this record).
     */
    struct StepPair {
        int mStep;    /*!< The step a position starts in. */
        int mPartner; /*!< The step the position is carried to. */
    };

    /**
     * Fill mStepRings from the script.
     *
     * Each of the kSetCount tables evaluates the script template 0x3a3 through EvalScriptTemplate()
     * with its index, which yields a sequence of step rings. Each ring pairs every step with the
     * one after it, and the last with the first. A step is read through Py::Int.
     *
     * @ghidraAddress NTSC-U/C: 0x00128410
     * @ghidraAddress PAL: 0x00128b40
     */
    void LoadStepRings();

    /**
     * Advance the window until GetExtent() passes the limit, then drop what it has passed.
     *
     * Every one of MapBar(), FindBarsPlaying(), GetPatternIndex(), GetAbsoluteSectionIndex(),
     * IsLooping(), EndLoop(), and StartLoop() calls it with its own argument before doing anything
     * else. The growth half appends one start to mWindowStarts from the result of GetExtent() and
     * one Entry to mWindow per section of the pattern. The test is at the top of the loop, and
     * the body can run zero times. GetExtent() is dispatched through the table rather than called
     * directly.
     *
     * The trim half drops as many leading elements from mWindow and mWindowStarts as the pattern
     * has, and adds that count to mTrimmedCount. Its guard compares a byte offset against an
     * element count. The trim therefore fires only once mWindow is more than eight times the length
     * of the pattern.
     *
     * @param nLimit The value GetExtent() must exceed for the growth to stop.
     * @ghidraAddress NTSC-U/C: 0x00129150
     * @ghidraAddress PAL: 0x00129880
     */
    void GrowPastLimit(int nLimit);

    /**
     * Grow the window past a bar and report the index of the entry it falls in.
     *
     * Inline. GetPatternIndex(), GetAbsoluteSectionIndex(), IsLooping(), EndLoop(), and
     * StartLoop() expand it, and MapBar() and SelfTest() call the out-of-line copy at
     * `0x0012ade8`.
     *
     * @param nBar The bar.
     * @return The index of the last window entry starting at or before the bar.
     */
    int WindowIndex(int nBar);

    /**
     * Recompute the start of every window entry from an index onward.
     *
     * Inline, and expanded in EndLoop() and StartLoop(). Each start is the previous start plus the
     * previous section's length times its repeat count.
     *
     * @param nFirst The first index to recompute.
     */
    void RestartFrom(std::vector<int>::size_type nFirst);

    // The window, one entry per section played.
    std::vector<Entry> mWindow; // +0x3c
    // The start position of each window entry.
    std::vector<int> mWindowStarts; // +0x48
    // Running count of elements the trim has dropped from the front of the two vectors above.
    int mTrimmedCount; // +0x54
    // The pattern RecordPattern() records and GrowPastLimit() appends again.
    std::vector<Entry> mPattern; // +0x58
    // The window end RecordPattern() records.
    int mPatternEnd; // +0x64
    // One partner table per set.
    std::vector<std::vector<StepPair> > mStepRings; // +0x68
};
