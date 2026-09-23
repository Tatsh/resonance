#pragma once

#include "met/fadeuser.h"
#include "met/metfade.h"
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
 * The constructor at `0x0028d2c8` takes only the renderer and the load priority. It supplies the
 * empty string at `0x007f5ca8` for the screen name, `metagame/Transition` for the directory, and
 * `loadgame` for the container. The empty screen name makes the two animation views resolve as
 * `_EE.anim` and `_BF.anim` with nothing before the underscore, which MetMemDetectStartup also
 * does. It zeroes `+0x94` through `+0xa8`, the pair at `+0xa0` in one 8-byte store, and then
 * builds mFade.
 *
 * It declares one virtual of its own at slot 39, at `0x0028e158`, and the name is not recovered.
 *
 * The object is 0xac bytes, which the constructor's zeroing run through `+0xa8` and the 4-byte
 * mFade pointer settle together. Nothing derives from the class, so no base offset in any
 * descriptor corroborates the total.
 *
 * The destructor at `0x00291a50` releases mFade and then runs the MetScreen destructor. MetFade
 * has no destructor of its own, which is why the release is a bare deallocator call with no null
 * test.
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
     * Silence the slide sound.
     *
     * Every one of the five overrides below is a two-instruction stub, so each was written inline
     * with an empty body. A transition screen plays no navigation sound.
     *
     * @ghidraAddress 0x00291620
     */
    virtual void PlaySlideSound(int) {
    }

    /**
     * Silence the leave sound.
     *
     * @ghidraAddress 0x00291628
     */
    virtual void PlayLeaveSound(int) {
    }

    /**
     * Silence the high sound.
     *
     * @ghidraAddress 0x00291630
     */
    virtual void PlayHighSound(int) {
    }

    /**
     * Silence the cycle-left sound.
     *
     * @ghidraAddress 0x00291638
     */
    virtual void PlayCycleLeftSound(int) {
    }

    /**
     * Silence the cycle-right sound.
     *
     * @ghidraAddress 0x00291640
     */
    virtual void PlayCycleRightSound(int) {
    }

private:
    int mUnknown90; // +0x90, not written by the constructor
    int mUnknown94; // +0x94
    int mUnknown98; // +0x98

public:
    /**
     * Written 1 by MetLogoScreen's slot 36 at `0x002baf20` before it pushes this screen for the
     * attract mode. Public because that write goes through the screen pointer directly. +0x9c
     */
    int mUnknown9c;

private:
    // Zeroed together in one 8-byte store. +0xa0 and +0xa4
    int mUnknowna0;
    int mUnknowna4;
    // The fade driver, built from the renderer. +0xa8
    MetFade *mFade;
};
