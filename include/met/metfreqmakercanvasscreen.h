#pragma once

#include "memcard/memcarduser.h"
#include "met/metscreen.h"

/**
 * Canvas of the FreQ maker.
 *
 * `24MetFreqMakerCanvasScreen` in the RTTI descriptor at `0x00902a60`, with two public non-virtual
 * bases at fixed offsets, MetScreen at `+0x00`, and MemcardUser at `+140`.
 *
 * The 39-entry primary vtable is at `0x007f1cd8`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The twenty-one-entry MemcardUser table at `0x007f1c28` adjusts `this` by `-140` in every entry.
 *
 * The constructor at `0x0025e5a8` takes only the renderer and the load priority, and supplies
 * `fm_canvas` for the screen name, `metagame/persona` for the directory, and `freq_maker_canvas`
 * for the container. It writes `+0x8c`, which is the MemcardUser vptr, and `+0x90`.
 *
 * The object is at least 0x94 bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor is at `0x00262030`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x0025e978`, 14 `0x002620d8`, 19 `0x002620c8`, 23 `0x00261f80`, 24 `0x00261f88`, 30
 * `0x002620d0`, 36 `0x0025e898`, 38 `0x0025e798`.
 */
class MetFreqMakerCanvasScreen : public MetScreen, public MemcardUser {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x0025e5a8
     */
    MetFreqMakerCanvasScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x00262030
     */
    virtual ~MetFreqMakerCanvasScreen();

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x00261f80
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x00261f88
     */
    virtual void PlayCycleRightSound(int nSelector);
};
