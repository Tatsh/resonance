#pragma once

#include "met/fadeuser.h"
#include "met/metscreen.h"

/**
 * Dialogue shown when the expansion hardware is absent.
 *
 * `21MetExpansionPakScreen` in the RTTI descriptor at `0x008efdd0`, with two public non-virtual
 * bases at fixed offsets, MetScreen at `+0x00`, and FadeUser at `+140`.
 *
 * The 39-entry primary vtable is at `0x007ebf30`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The four-entry FadeUser table at `0x007ebf08` adjusts `this` by `-140` in every entry.
 *
 * The constructor at `0x00218320` takes only the renderer and the load priority, and supplies
 * `dlg` for the screen name, `metagame/Shared` for the directory, and `dialogue` for the
 * container. It writes `+0x8c`, which is the FadeUser vptr, and `+0xc0`.
 *
 * It is the second of only two classes that fill slot 16, the other being MetRemixDelScreen.
 *
 * The object is at least 0xc4 bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor is at `0x0021d8a8`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x0021d948`, 9 `0x0021d9a8`, 15 `0x002193e8`, 16 `0x0021d9e0`, 23 `0x0021d810`, 24
 * `0x0021d818`, 26 `0x00218518`, 38 `0x0021d928`.
 */
class MetExpansionPakScreen : public MetScreen, public FadeUser {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00218320
     */
    MetExpansionPakScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x0021d8a8
     */
    virtual ~MetExpansionPakScreen();

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x0021d810
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x0021d818
     */
    virtual void PlayCycleRightSound(int nSelector);
};
