#pragma once

#include "met/metkbuser.h"
#include "met/metscreen.h"

/**
 * Button row of the FreQ maker.
 *
 * `25MetFreqMakerButtonsScreen` in the RTTI descriptor at `0x00902a50`, with two public non-virtual
 * bases at fixed offsets, MetScreen at `+0x00`, and MetKBUser at `+140`.
 *
 * The 39-entry primary vtable is at `0x007f1560`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The three-entry MetKBUser table at `0x007f1540` adjusts `this` by `-140` in every entry.
 *
 * The constructor at `0x00257968` takes only the renderer and the load priority, and supplies
 * `fm_buttons` for the screen name, `metagame/persona` for the directory, and `freq_maker_buttons`
 * for the container. It writes `+0x8c`, which is the MetKBUser vptr, then `+0x90`, `+0x94`, and
 * `+0x9c`.
 *
 * It is one of only five classes that override slot 14, the container-load poll.
 *
 * The object is at least 0xa0 bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor is at `0x0025e108`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x00258c80`, 7 `0x0025e1e0`, 14 `0x00259fa0`, 15 `0x00259ad0`, 19 `0x002581c8`, 23
 * `0x0025e070`, 24 `0x0025e078`, 30 `0x00258f40`, 33 `0x0025e1a0`, 36 `0x00259040`, 38
 * `0x00257b70`.
 */
class MetFreqMakerButtonsScreen : public MetScreen, public MetKBUser {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00257968
     */
    MetFreqMakerButtonsScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x0025e108
     */
    virtual ~MetFreqMakerButtonsScreen();

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x0025e070
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x0025e078
     */
    virtual void PlayCycleRightSound(int nSelector);
};
