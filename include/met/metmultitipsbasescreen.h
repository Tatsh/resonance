#pragma once

#include "met/metscreen.h"

/**
 * Base of the five loading-tip screens.
 *
 * `22MetMultiTipsBaseScreen` in the RTTI descriptor at `0x008ef920`, with MetScreen as its one
 * public non-virtual base at offset 0. The 39-entry vtable is at `0x00801610`, the same length as
 * the MetScreen table, so the class declares no virtual of its own.
 *
 * Five classes derive from the class, MetMultiTips1Screen through MetMultiTips5Screen. Each child
 * has an accessor-sized GetTypeInfo of its own at `0x0030d880`, `0x0030da10`, `0x0030dba0`,
 * `0x0030dd30`, and `0x0030dec0`, and each child constructor writes the shared vtable before its
 * own, which is how the constructor of this class is known to be inline.
 *
 * The size is not recovered. No child places a second base after the subobject, so nothing in the
 * RTTI fixes the width, and it is therefore at least the 0x8c of MetScreen.
 *
 * Nine slots differ from the MetScreen table. Slots 22, 23, and 24 are two-instruction `jr ra`
 * stubs, so this class silences three of the six MetScreen sounds by overriding them with an empty
 * body. None of the remaining five has a recovered name.
 *
 *  - 1 `0x0030d710` the destructor.
 *  - 5 `0x003070f0` replaces the show-and-animate routine at `0x003900a8`.
 *  - 9 `0x0030d7d0` replaces the start-exit routine at `0x00390100`.
 *  - 19 `0x00306ee0` replaces an empty MetScreen slot.
 *  - 22 `0x0030d7a0` PlayHighSound(), overridden empty.
 *  - 23 `0x0030d790` PlayCycleLeftSound(), overridden empty.
 *  - 24 `0x0030d798` PlayCycleRightSound(), overridden empty.
 *  - 33 `0x0030d7a8` replaces an empty MetScreen slot with a non-empty body.
 *  - 36 `0x003072a0` replaces an empty MetScreen slot.
 */
class MetMultiTipsBaseScreen : public MetScreen {
public:
    /**
     * @ghidraAddress 0x0030d710
     */
    virtual ~MetMultiTipsBaseScreen();

    /**
     * @ghidraAddress 0x0030d7a0
     */
    virtual void PlayHighSound();

    /**
     * @ghidraAddress 0x0030d790
     */
    virtual void PlayCycleLeftSound();

    /**
     * @ghidraAddress 0x0030d798
     */
    virtual void PlayCycleRightSound();
};
