#pragma once

#include "met/metscreen.h"

/**
 * Screen that swaps five of its six sounds for the in-game multiplayer bank.
 *
 * `23MetScreenMultiSoundBank` in the RTTI descriptor at `0x008efda0`, with MetScreen as its one
 * public non-virtual base at offset 0. The class declares no data member, which
 * MetConfigControllerScreen proves by placing its MemcardUser base at `+140`, the same offset a
 * direct MetScreen child uses. The 39-entry vtable is at `0x0080b560`.
 *
 * Three classes derive from the class, MetConfigGameOptionsScreen, MetMsgScreen, and
 * MetPauseBaseScreen.
 *
 * The constructor at `0x0038ff58` forwards all five arguments to MetScreen unchanged and then
 * writes its own vptr. The destructor at `0x0038fe60` runs the MetScreen destructor and has no
 * body of its own.
 *
 * Every override tests the predicate at `0x003908f0`, which reports whether the multiplayer
 * in-game bank applies, and plays one of two named sounds through the player at `0x0012f470`. The
 * predicate has no recovered name, so it is recorded rather than declared. Four slots pick
 * `SND_MET_MULTI_INGAME_ACTION` or `SND_MET_MULTI_INGAME_NAVIGATION` as the alternative, and the
 * error sound at slot 25 is the one of the six that stays as MetScreen defined it.
 *
 *  - 20 `0x003907b0` `SND_MET_MULTI_INGAME_ACTION`, otherwise `SND_MET_SLIDE`.
 *  - 21 `0x003907f0` `SND_MET_MULTI_INGAME_ACTION`, otherwise `SND_MET_LEAVE`.
 *  - 22 `0x00390830` `SND_MET_MULTI_INGAME_NAVIGATION`, otherwise `SND_MET_HIGH`.
 *  - 23 `0x00390870` `SND_MET_MULTI_INGAME_NAVIGATION`, otherwise `SND_MET_CYCLE_L`.
 *  - 24 `0x003908b0` `SND_MET_MULTI_INGAME_NAVIGATION`, otherwise `SND_MET_CYCLE_R`.
 */
class MetScreenMultiSoundBank : public MetScreen {
public:
    /**
     * Construct a screen that uses the multiplayer sound bank.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @param name The screen name.
     * @param directory The directory the container loads from.
     * @param file The container name, without the `.rnd` suffix.
     * @ghidraAddress 0x0038ff58
     */
    MetScreenMultiSoundBank(MetRenderer *pRenderer,
                            int nPriority,
                            const HxStr &name,
                            const HxStr &directory,
                            const HxStr &file);

    /**
     * @ghidraAddress 0x0038fe60
     */
    virtual ~MetScreenMultiSoundBank();

    /**
     * @ghidraAddress 0x003907b0
     */
    virtual void PlaySlideSound();

    /**
     * @ghidraAddress 0x003907f0
     */
    virtual void PlayLeaveSound();

    /**
     * @ghidraAddress 0x00390830
     */
    virtual void PlayHighSound();

    /**
     * @ghidraAddress 0x00390870
     */
    virtual void PlayCycleLeftSound();

    /**
     * @ghidraAddress 0x003908b0
     */
    virtual void PlayCycleRightSound();
};
