#pragma once

#include "met/fadeuser.h"
#include "met/metscreen.h"

/**
 * Transition screen shown while a game loads.
 *
 * `17MetLoadGameScreen` in the RTTI descriptor at `0x00901f00`, with two public non-virtual bases
 * at fixed offsets, MetScreen at `+0x00`, and FadeUser at `+140`.
 *
 * The 40-entry primary vtable is at `0x007f5f18`, one entry longer than the MetScreen table, so
 * the class declares one virtual of its own, at slot 39.
 *
 * The four-entry FadeUser table at `0x007f5ef0` adjusts `this` by `-140` in every entry.
 *
 * The constructor at `0x0028d2c8` takes only the renderer and the load priority. It supplies
 * `metagame/Transition` for the directory and `loadgame` for the container. It writes `+0x8c`,
 * which is the FadeUser vptr, then `+0x94`, `+0x98`, `+0x9c`, and `+0xa8`.
 *
 * It declares one virtual of its own at slot 39, at `0x0028e158`, and the name is not recovered.
 * Its screen name is not a literal the constructor loads, unlike every other leaf, so it is not
 * recovered here.
 *
 * The object is at least 0xac bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor is at `0x00291a50`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x0028d590`, 20 `0x00291620`, 21 `0x00291628`, 22 `0x00291630`, 23 `0x00291638`, 24
 * `0x00291640`, 26 `0x0028dd38`, 33 `0x0028dc60`, 38 `0x0028d4c0`, 39 `0x0028e158`.
 */
class MetLoadGameScreen : public MetScreen, public FadeUser {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x0028d2c8
     */
    MetLoadGameScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x00291a50
     */
    virtual ~MetLoadGameScreen();

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x00291620
     */
    virtual void PlaySlideSound(int nSelector);

    /**
     * @ghidraAddress 0x00291628
     */
    virtual void PlayLeaveSound();

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x00291630
     */
    virtual void PlayHighSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x00291638
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x00291640
     */
    virtual void PlayCycleRightSound(int nSelector);
};
