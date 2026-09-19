#pragma once

#include <cstddef>
#include <vector>

#include "rnd/object.h"

/**
 * Ring of front-end buttons with one of them selected.
 *
 * `13MetButtonList` in the RTTI descriptor at `0x0086f5f8`, a leaf class with no base. Following
 * the g++ 2.x layout for a class with no base, the vptr sits after the data members at `+0x14`,
 * and the object is 0x18 bytes. The five-entry vtable is at `0x007e90f8`.
 *
 * The class supplies its own allocation function, which tags every instance with the literal
 * `MetButtonList` at `0x007e8e10`. That is the one place in the foundation where a Met class
 * declares `operator new`.
 *
 * Both navigation virtuals walk mButtons in their own direction, wrap at the end, and skip an
 * entry whose field at `+0x1c` equals 3, which is how a disabled button is passed over. Both stop
 * after visiting every entry, so a list in which every button is disabled retains the mSelected
 * value it had before the call.
 *
 * The four declared virtuals are the destructor at `0x001fca30`, the two navigation routines at
 * `0x001fcc40` and `0x001fcd10`, and the selection-change notification at `0x001feed8`. None of
 * the four has a recovered name, so all four are recorded rather than declared apart from the
 * destructor.
 *
 * The constructor at `0x001fc9f8` is inline. It writes the vptr, zeroes the first word and the
 * vector, and sets mSelected to -1, which is the sentinel for no selection that the
 * selection-change routine tests for.
 */
class MetButtonList {
public:
    /**
     * Allocate an instance from the tagged heap.
     *
     * @param nSize The object size, which the compiler supplies.
     * @return The block.
     * @ghidraAddress 0x001fecc8
     */
    void *operator new(size_t nSize);

    /**
     * Release every button reference and the vector.
     *
     * @ghidraAddress 0x001fca30
     */
    virtual ~MetButtonList();

private:
    int mUnknown00;                      // +0x00
    std::vector<Rnd::Object *> mButtons; // +0x04

public:
    /**
     * Index of the selected button, or -1 for none.
     *
     * Public rather than private, because MetLoadFreqBaseScreen reads it directly and the image
     * has no accessor to route that read through. A friend declaration fits the image equally
     * well. The member is declared after the two private ones so that the recovered order, and
     * therefore the recovered layout, is preserved. +0x10
     */
    int mSelected;
};
