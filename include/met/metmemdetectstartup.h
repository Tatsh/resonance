#pragma once

#include "met/fadeuser.h"
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
 * nothing before the underscore. It then allocates a scalar block into mUnknowna4 through the
 * routine at `0x0016a1c8`.
 *
 * The destructor at `0x002e2eb8` releases that block through MemFreeScalar, restores the FadeUser
 * vptr to `0x007ec070`, runs the MetMemDetectScreen destructor, and releases the object with the
 * tag `MsgSink`.
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
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x002e2e00
     */
    virtual void PlaySlideSound(int nSelector);

    /**
     * @ghidraAddress 0x002e2e08
     */
    virtual void PlayLeaveSound();

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x002e2e10
     */
    virtual void PlayHighSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x002e2e18
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x002e2e20
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x002e2e28
     */
    virtual void PlayErrorSound(int nSelector);

private:
    int mUnknowna4; // +0xa4, a scalar block the destructor releases
    int mUnknowna8; // +0xa8
};
