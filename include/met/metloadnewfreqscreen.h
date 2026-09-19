#pragma once

#include "met/metkbuser.h"
#include "met/metloadfreqbasescreen.h"

/**
 * Screen that creates a new FreQ identity and takes its name from the keyboard.
 *
 * `20MetLoadNewFreqScreen` in the RTTI descriptor at `0x008f08d0`, with two public non-virtual
 * bases at fixed offsets, MetLoadFreqBaseScreen at `+0x00` and MetKBUser at `+164`. The class
 * declares no data member, so the object is 0xa8 bytes. The 47-entry primary vtable is at
 * `0x007f83a8` and the three-entry MetKBUser table at `0x007f8388` adjusts `this` by `-164`. That
 * table is where this screen supplies the one MetKBUser pure virtual. The primary is the same
 * length as the MetLoadFreqBaseScreen table, so the class declares no virtual of its own.
 *
 * The constructor at `0x002a8418` takes only the renderer and the load priority and runs the
 * MetLoadFreqBaseScreen constructor at `0x00291e00`. The destructor at `0x002a8458` restores the
 * primary vptr, restores the MetKBUser vptr to `0x007f16a0`, runs the MetLoadFreqBaseScreen
 * destructor, and releases the object with the tag `MsgSink`.
 *
 * Twelve slots differ from the MetLoadFreqBaseScreen table. This is the only class in the
 * subsystem that overrides slot 11, at `0x002a3978`, which every other class inherits as a
 * two-instruction stub. Of the rest only the destructor has a recovered name, and the others are
 * 5 `0x002a3890`, 9 `0x002a84c0`, 15 `0x002a4910`, 39 `0x002a85c0`, 40 `0x002a4108`,
 * 41 `0x002a3f80`, 43 `0x002a8670`, 44 `0x002a8590`, and 45 `0x002a3b08`.
 */
class MetLoadNewFreqScreen : public MetLoadFreqBaseScreen, public MetKBUser {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002a8418
     */
    MetLoadNewFreqScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x002a8458
     */
    virtual ~MetLoadNewFreqScreen();
};
