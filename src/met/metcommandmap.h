#pragma once

#include <map>
#include <vector>

#include "met/metscreen.h"
#include "msg/metcontrollerreading.h"

/**
 * Translator from one raw controller reading to one front-end command.
 *
 * The class emits no RTTI, so no descriptor, accessor, or vtable in the image bears its name. It
 * declares no allocation operator of its own either, because MetRenderer's constructor allocates
 * it through the tagged allocator with an explicit size rather than through a class operator, so
 * the deallocation-tag lever does not reach it. Its translation unit spans roughly `0x002e33f0`
 * through `0x002e72bc`, between MetMemDetectStartup and MetModeScreen, and no literal in that span
 * identifies a file or a function. The name here is therefore inferred from what the class does and
 * is not attested anywhere in the image.
 *
 * The object is twelve bytes and its one member is a `std::vector` of five `std::map<int, int>`
 * records, one per controller index, which remember which analogue directions are held so that a
 * held stick yields one command rather than one per reading. The map's twenty-four byte nodes and
 * the `stl_maptree` tag the constructor sets fix the record.
 *
 * The destructor is emitted into MetRenderer's translation unit rather than the class's own, which
 * is what an inline destructor compiles to.
 */
class MetCommandMap {
public:
    /**
     * Build the five empty held-direction records.
     *
     * @ghidraAddress 0x002e33f0
     */
    MetCommandMap();

    /**
     * Release every record and the vector.
     *
     * Inline. The address is its out-of-line copy in MetRenderer's translation unit.
     *
     * @ghidraAddress 0x00371138
     */
    ~MetCommandMap() {
    }

    /**
     * Translate one controller reading into one front-end command.
     *
     * The command's mPadIndex takes the reading's. A `key ` reading whose mValue is above zero
     * yields command 15 with the reading's mButton. A `joy ` reading dispatches on mButton through
     * a 103-entry table. Buttons 1 through 16 yield a fixed code while pressed and 0 when released,
     * buttons 100 through 103 are the four analogue directions and go through AxisCommand(), and
     * every other button yields -1 and sets mPadIndex to 1. A reading of any other tag leaves
     * mCommand as it was.
     *
     * @param pReading The reading, which is the RawControllerMsg payload rather than the message.
     * @param pCommand The command the reading translates to.
     * @return Non-zero unless the command is -1.
     * @ghidraAddress 0x002e3738
     */
    int Translate(const MetControllerReading *pReading, MetScreenCommand *pCommand);

private:
    // 0x002e3ac0
    // Translates one analogue reading. A value below 0.1 yields nNegative and one above 0.9 yields
    // nPositive, each only once until the stick returns to the centre, where the direction is
    // released and 0 is returned. A direction already held returns -1.
    int AxisCommand(int nButton, int nPadIndex, float flValue, int nNegative, int nPositive);

    // Which directions each controller holds, by button. +0x00
    std::vector<std::map<int, int> > mHeld;
};
