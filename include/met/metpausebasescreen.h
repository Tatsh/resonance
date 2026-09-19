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
 * The constructor is at `0x00317d40`. It takes the renderer, the load priority, the screen name,
 * the directory, the container name, and the screen registry key, and all four children call it,
 * each passing its own class name verbatim as the key. An earlier pass read the single extra
 * writer of this vtable as evidence that the constructor was inline; that writer is the
 * constructor.
 *
 * The size is at least 0xb4. MetPauseSoloGameScreen and MetPauseSoloRemixScreen both zero the word
 * at `+0xb0` in their constructors and the other two children do not, so that word is either a
 * protected member of this class that two children reset or a member of each of those two, and the
 * two cannot be told apart from the constructors alone.
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
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x0031bff0
     */
    virtual void PlaySlideSound(int nSelector);

    /**
     * @ghidraAddress 0x0031bff8
     */
    virtual void PlayLeaveSound();

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x0031c000
     */
    virtual void PlayHighSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x0031c008
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x0031c010
     */
    virtual void PlayCycleRightSound(int nSelector);
};
