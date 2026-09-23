#pragma once

#include "met/fadeuser.h"
#include "met/metfade.h"
#include "met/metmemdetectscreen.h"

/**
 * First memory-card probe, run during start-up.
 *
 * `19MetMemDetectStartup` in the RTTI descriptor at `0x008eff90`, with two public non-virtual
 * bases at fixed offsets, MetMemDetectScreen at `+0x00` and FadeUser at `+160`. The object is 0xac
 * bytes.
 *
 * Three vtables belong to the class, the 44-entry primary at `0x007fd518`, the four-entry FadeUser
 * table at `0x007fd440` that adjusts `this` by `-160`, and the 21-entry MemcardUser table at
 * `0x007fd468` that adjusts it by `-140`. The FadeUser table is where this class supplies the two
 * FadeUser pure virtuals. The primary is the same length as the MetMemDetectScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x002df058` takes only the renderer and the load priority. It runs the
 * MetMemDetectScreen constructor with an **empty** screen name, `metagame/Shared` for the
 * directory, and `memdetect1` for the container. The empty screen name is the one in the
 * subsystem, and it makes the two animation views resolve as `_EE.anim` and `_BF.anim` with
 * nothing before the underscore. MetLoadGameScreen uses an empty screen name as well. The
 * constructor then builds mFade, the same MetFade that MetLoadGameScreen builds.
 *
 * The destructor at `0x002e2eb8` releases mFade through the scalar deallocator with no null test,
 * which is what a delete expression compiles to for a class with no destructor, restores the
 * FadeUser vptr to `0x007ec070`, runs the MetMemDetectScreen destructor, and releases the object
 * with the tag `MsgSink`.
 *
 * Twelve slots differ from the MetMemDetectScreen table. Slots 20 through 25 sit eight bytes apart
 * at `0x002e2e00` through `0x002e2e28` and are two-instruction `jr ra` stubs, so this screen plays
 * none of the six MetScreen sounds, the only class in the subsystem that silences all six. Of the
 * rest only the destructor has a recovered name, and the others are 5 `0x002e2f40`,
 * 9 `0x002e2fb8`, 26 `0x002df250`, 39 `0x002df6e0`, 41 `0x002e3058`, and 42 `0x002e3068`.
 */
class MetMemDetectStartup : public MetMemDetectScreen, public FadeUser {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002df058
     */
    MetMemDetectStartup(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x002e2eb8
     */
    virtual ~MetMemDetectStartup();

    /**
     * Build the screen on the heap.
     *
     * MetScreen::CreateStartupScreens() registers this factory. It allocates 0xb0 bytes.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x002e2e30
     */
    static MetMemDetectStartup *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Respond to a MetFade fade out having finished.
     *
     * FadeUser slot 2, through the FadeUser table entry that adjusts `this` by `-160`.
     *
     * @ghidraAddress 0x002e30f0
     */
    virtual void OnFadeOutDone();

    /**
     * Respond to a MetFade fade in having finished.
     *
     * FadeUser slot 3, through the FadeUser table entry that adjusts `this` by `-160`.
     *
     * @ghidraAddress 0x002e30a0
     */
    virtual void OnFadeInDone();

    /**
     * Silence the slide sound.
     *
     * All six overrides are two-instruction stubs, so each was written inline with an empty body.
     * This is the one screen in the subsystem that silences every MetScreen sound.
     *
     * @ghidraAddress 0x002e2e00
     */
    virtual void PlaySlideSound(int) {
    }

    /**
     * Silence the leave sound.
     *
     * @ghidraAddress 0x002e2e08
     */
    virtual void PlayLeaveSound(int) {
    }

    /**
     * Silence the high sound.
     *
     * @ghidraAddress 0x002e2e10
     */
    virtual void PlayHighSound(int) {
    }

    /**
     * Silence the cycle-left sound.
     *
     * @ghidraAddress 0x002e2e18
     */
    virtual void PlayCycleLeftSound(int) {
    }

    /**
     * Silence the cycle-right sound.
     *
     * @ghidraAddress 0x002e2e20
     */
    virtual void PlayCycleRightSound(int) {
    }

    /**
     * Silence the error sound.
     *
     * @ghidraAddress 0x002e2e28
     */
    virtual void PlayErrorSound(int) {
    }

private:
    MetFade *mFade; // +0xa4
    int mUnknowna8; // +0xa8
};
