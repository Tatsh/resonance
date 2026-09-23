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
 * Slot20() appends one section, and Slot19() records the list as the pattern GrowPastLimit()
 * appends again whenever a position runs past the end. A repeat count of kRepeatForever marks a
 * section that loops until Slot16() ends it at a bar, and Slot17() sets a section looping again.
 *
 * The unit registers SelfTest() with TestRegistry under the name `PlayMapLinear`.
 */
class PlayMapLinear : public PlayMap {
public:
    /**
     * Construct an empty linear map.
     *
     * Runs the PlayMap constructor, sizes mUnknown68 to kSetCount empty tables, and reserves eight
     * elements in mUnknown3c and mUnknown48. A non-zero bLoadStepRings then runs LoadStepRings().
     * LevelBuilder's constructor passes 1, and SelfTest() passes 0.
     *
     * @param bLoadStepRings Whether to fill the partner tables from the script.
     * @ghidraAddress 0x00127a80
     */
    explicit PlayMapLinear(int bLoadStepRings);

    /** @ghidraAddress 0x0012a4d8 */
    virtual ~PlayMapLinear();

    /**
     * Map a position to its offset within the base's steps.
     *
     * Finds the window entry the position falls in and returns that section's first step plus the
     * position's distance into the entry, wrapped to the section's length.
     *
     * @param nValue The position to map.
     * @return The mapped position.
     * @ghidraAddress 0x0012ad58
     */
    virtual int Slot5(int nValue);

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
     * @return mUnknown2c.
     * @ghidraAddress 0x00128d08
     */
    virtual std::vector<int> &Slot6(int nStart, int nMin, int nEnd);

    /**
     * Carry a position from its step to the partner step the table for one set records.
     *
     * The position's step comes from PlayMap::FindStepIndex(). Without a record for that step in
     * mUnknown68[nSet] the position comes back unchanged.
     *
     * @param nValue The position to map.
     * @param nSet The index into mUnknown68.
     * @return The mapped position.
     * @ghidraAddress 0x0012ae88
     */
    virtual int Slot7(int nValue, int nSet);

    /**
     * Report the position at which the window ends.
     *
     * @return The start of the last window entry plus its section's length times its repeat count,
     *         or zero when the window is empty.
     * @ghidraAddress 0x0012ae38
     */
    virtual int Slot8();

    /**
     * @return The window end Slot19() recorded.
     * @ghidraAddress 0x0012aa60
     */
    virtual int Slot9();

    /**
     * @return The number of sections in the recorded pattern.
     * @ghidraAddress 0x0012aa68
     */
    virtual int Slot10();

    /**
     * @param nValue The index into the recorded pattern.
     * @return The section at that index.
     * @ghidraAddress 0x0012aa80
     */
    virtual int Slot11(int nValue);

    /**
     * Report the index within the recorded pattern of the window entry a position falls in.
     *
     * @param nValue The position.
     * @return The window index modulo the pattern length.
     * @ghidraAddress 0x0012af30
     */
    virtual int Slot12(int nValue);

    /**
     * Report the absolute index of the window entry a position falls in.
     *
     * @param nValue The position.
     * @return The window index plus the count of entries the trim has dropped.
     * @ghidraAddress 0x0012afb8
     */
    virtual int Slot13(int nValue);

    /**
     * @param nValue The position.
     * @return 1 when the window entry the position falls in repeats forever, and 0 otherwise.
     * @ghidraAddress 0x0012b028
     */
    virtual int Slot14(int nValue);

    /**
     * End the looping section at a bar.
     *
     * When the window entry the bar falls in repeats forever, its repeat count becomes the number
     * of whole passes up to and including the bar, and every later entry is moved to follow it.
     *
     * @param nBar The bar.
     * @return 1 when the entry was looping, and 0 otherwise.
     * @ghidraAddress 0x00128ed8
     */
    virtual int Slot16(int nBar);

    /**
     * Set the section a bar falls in looping forever, and move every later entry to follow it.
     *
     * @param nBar The bar.
     * @return Always 1.
     * @ghidraAddress 0x00129038
     */
    virtual int Slot17(int nBar);

    /**
     * Toggle the loop on the section a bar falls in.
     *
     * @param nBar The bar.
     * @return 1 when a loop was ended, and 0 when one was started.
     * @ghidraAddress 0x0012b0a8
     */
    virtual int Slot18(int nBar);

    /**
     * Record the sections appended so far as the pattern, and the window end with it.
     *
     * @ghidraAddress 0x0012aa18
     */
    virtual void Slot19();

    /**
     * Slot 20. Append one section, played once, at the end of the window.
     *
     * LevelBuilder's constructor calls it once per value of configuration code 0x39d.
     *
     * @param nSection The index of the section's first step.
     * @ghidraAddress 0x00128c38
     */
    virtual void Slot20(int nSection);

    /**
     * Exercise the map on four steps and seven sections.
     *
     * The routine maps seven positions and toggles two loops, and discards every result. Nothing in
     * the shipped game calls it apart from the test registry.
     *
     * @return Always 1.
     * @ghidraAddress 0x001293b0
     */
    static int SelfTest();

    /**
     * Run SelfTest() in the shape TestRegistry::TestFunc requires, discarding its result.
     *
     * The unit's static initialiser registers it.
     *
     * @ghidraAddress 0x0012b110
     */
    static void RunSelfTest();

protected:
    /** The number of partner tables in mUnknown68. */
    static constexpr int kSetCount = 8;

    /** The repeat count that marks a section looping until Slot16() ends it. */
    static constexpr int kRepeatForever = 10000;

    /** One section of the window, a step index paired with its repeat count. Eight bytes. */
    struct Entry {
        int mSection; /*!< The index into mSteps and mSectionLengths. */
        int mRepeats; /*!< The number of passes, or kRepeatForever. */
    };

    /**
     * One partner record, a step paired with the step Slot7() carries it to. Eight bytes.
     *
     * A type distinct from Entry, because the two vectors grow through separate insertion routines
     * (`0x0012a238` for Entry and `0x00129d68` for this record).
     */
    struct StepPair {
        int mStep;    /*!< The step a position starts in. */
        int mPartner; /*!< The step the position is carried to. */
    };

    /**
     * Fill mUnknown68 from the script.
     *
     * Each of the kSetCount tables evaluates the script template 0x3a3 through EvalScriptTemplate()
     * with its index, which yields a sequence of step rings. Each ring pairs every step with the
     * one after it, and the last with the first. A step is read through Py::Int.
     *
     * @ghidraAddress 0x00128410
     */
    void LoadStepRings();

    /**
     * Advance the window until slot 8 passes the limit, then drop what it has passed.
     *
     * Every one of slots 5, 6, 12, 13, 14, 16, and 17 calls it with its own argument before doing
     * anything else. The growth half appends one start to mUnknown48 from slot 8's result and one
     * Entry to mUnknown3c per section of the pattern, and the test is at the top of the loop so the
     * body can run zero times. Slot 8 is dispatched through the table rather than called directly.
     *
     * The trim half drops as many leading elements from mUnknown3c and mUnknown48 as the pattern
     * holds, and adds that count to mUnknown54. Its guard compares a byte offset against an element
     * count, which both the disassembly and the decompiler agree on, so the trim fires only once
     * mUnknown3c is more than eight times the length of the pattern.
     *
     * @param nLimit The value slot 8 must exceed for the growth to stop.
     * @ghidraAddress 0x00129150
     */
    void GrowPastLimit(int nLimit);

    /**
     * Grow the window past a position and report the index of the entry it falls in.
     *
     * Inline. Slots 12, 13, 14, 16, and 17 expand it, and Slot5() and SelfTest() call the
     * out-of-line copy at `0x0012ade8`.
     *
     * @param nValue The position.
     * @return The index of the last window entry starting at or before the position.
     */
    int WindowIndex(int nValue);

    /**
     * Recompute the start of every window entry from an index onward.
     *
     * Inline, and expanded in Slot16() and Slot17(). Each start is the previous start plus the
     * previous section's length times its repeat count.
     *
     * @param nFirst The first index to recompute.
     */
    void RestartFrom(std::vector<int>::size_type nFirst);

    // The window, one entry per section played.
    std::vector<Entry> mUnknown3c; // +0x3c
    // The start position of each window entry.
    std::vector<int> mUnknown48; // +0x48
    // Running count of elements the trim has dropped from the front of the two vectors above.
    int mUnknown54; // +0x54
    // The pattern Slot19() records and GrowPastLimit() appends again.
    std::vector<Entry> mUnknown58; // +0x58
    // The window end Slot19() records.
    int mUnknown64; // +0x64
    // One partner table per set.
    std::vector<std::vector<StepPair> > mUnknown68; // +0x68
};
