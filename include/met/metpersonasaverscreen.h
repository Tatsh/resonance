#pragma once

#include "memcard/memcarduser.h"
#include "met/metkbuser.h"
#include "met/metscreen.h"

/**
 * Dialogue that writes a persona to a memory card.
 *
 * `21MetPersonaSaverScreen` in the RTTI descriptor at `0x008f0060`, with three public non-virtual
 * bases at fixed offsets, MetScreen at `+0x00`, MemcardUser at `+140`, and MetKBUser at `+144`.
 *
 * The 39-entry primary vtable is at `0x00805600`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The twenty-one-entry MemcardUser table at `0x00805550` adjusts `this` by `-140` in every entry.
 *
 * The three-entry MetKBUser table at `0x00805530` adjusts `this` by `-144` in every entry.
 *
 * The constructor at `0x0032ece0` takes only the renderer and the load priority, and supplies
 * `dlg` for the screen name, `metagame/Shared` for the directory, and `dialogue` for the
 * container. It writes `+0x8c` and `+0x90`, which are the two secondary vptrs, then `+0x98`,
 * `+0x9c`, a vector at `+0xa0`, a second at `+0xac`, then `+0xbc` and `+0xc0`.
 *
 * The object is at least 0xc4 bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor is at `0x0032f020`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x003390c0`, 7 `0x003390f0`, 9 `0x00339110`, 15 `0x003346c8`, 23 `0x00338f88`, 24
 * `0x00338f90`, 38 `0x003390a0`.
 */
class MetPersonaSaverScreen : public MetScreen, public MemcardUser, public MetKBUser {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x0032ece0
     */
    MetPersonaSaverScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x0032f020
     */
    virtual ~MetPersonaSaverScreen();

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x00338f88
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x00338f90
     */
    virtual void PlayCycleRightSound(int nSelector);
};
