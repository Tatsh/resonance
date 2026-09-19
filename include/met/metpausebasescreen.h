#pragma once

#include "met/metscreenmultisoundbank.h"

/**
 * Base of the four pause screens.
 *
 * `18MetPauseBaseScreen` in the RTTI descriptor at `0x00902300`, with MetScreenMultiSoundBank as
 * its one public non-virtual base at offset 0. The vtable is at `0x00802be8` and has 40 entries,
 * one more than the MetScreen table, so the class declares exactly one virtual of its own at slot
 * 39. That body is at `0x0031c018` and has no recovered name, so it is recorded rather than
 * declared.
 *
 * Four classes derive from the class, MetPauseGameScreen, MetPauseMultiRemixScreen,
 * MetPauseSoloGameScreen, and MetPauseSoloRemixScreen.
 *
 * The size is not recovered. No child places a second base after the subobject, so nothing in the
 * RTTI fixes the width, and it is therefore at least the 0x8c of MetScreenMultiSoundBank. The
 * constructor is inline: only the routine at `0x00317d40` and the destructor write this vtable.
 *
 * Eleven slots differ from the MetScreenMultiSoundBank table. Slots 20 through 24 are
 * two-instruction `jr ra` stubs, so a pause screen plays none of the five sounds its base swapped
 * for the multiplayer bank. None of the four non-sound overrides has a recovered name.
 *
 *  - 1 `0x00317e90` the destructor.
 *  - 5 `0x00318278` replaces the show-and-animate routine at `0x003900a8`.
 *  - 15 `0x00318b80` replaces an empty MetScreen slot.
 *  - 19 `0x00318010` replaces an empty MetScreen slot.
 *  - 20 `0x0031bff0` PlaySlideSound(), overridden empty.
 *  - 21 `0x0031bff8` PlayLeaveSound(), overridden empty.
 *  - 22 `0x0031c000` PlayHighSound(), overridden empty.
 *  - 23 `0x0031c008` PlayCycleLeftSound(), overridden empty.
 *  - 24 `0x0031c010` PlayCycleRightSound(), overridden empty.
 *  - 36 `0x00318418` replaces an empty MetScreen slot.
 *  - 39 `0x0031c018` the one virtual this class declares.
 */
class MetPauseBaseScreen : public MetScreenMultiSoundBank {
public:
    /**
     * @ghidraAddress 0x00317e90
     */
    virtual ~MetPauseBaseScreen();

    /**
     * @ghidraAddress 0x0031bff0
     */
    virtual void PlaySlideSound();

    /**
     * @ghidraAddress 0x0031bff8
     */
    virtual void PlayLeaveSound();

    /**
     * @ghidraAddress 0x0031c000
     */
    virtual void PlayHighSound();

    /**
     * @ghidraAddress 0x0031c008
     */
    virtual void PlayCycleLeftSound();

    /**
     * @ghidraAddress 0x0031c010
     */
    virtual void PlayCycleRightSound();
};
