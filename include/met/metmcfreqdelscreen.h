#pragma once

#include "memcard/memcarduser.h"
#include "met/listdataprovider.h"
#include "met/metmemcardpickeruser.h"
#include "met/metscreen.h"

/**
 * Screen that lists the saved FreQs on a memory card.
 *
 * The class name and the container name disagree. The RTTI titles the class for deletion and the
 * constructor loads `memcard_freq_load`, and both are reproduced as the image records them rather
 * than reconciled.
 *
 * `18MetMCFreqDelScreen` in the RTTI descriptor at `0x00901ea0`, with four public non-virtual bases
 * at fixed offsets, MetScreen at `+0x00`, MemcardUser at `+140`, ListDataProvider at `+144`, and
 * MetMemCardPickerUser at `+148`.
 *
 * The 39-entry primary vtable is at `0x007fabe0`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The twenty-one-entry MemcardUser table at `0x007fab30` adjusts `this` by `-140` in every entry.
 *
 * The four-entry ListDataProvider table at `0x007fab08` adjusts `this` by `-144` in every entry.
 *
 * The constructor at `0x002be968` takes only the renderer and the load priority, and supplies
 * `mcfl` for the screen name, `metagame/Shared` for the directory, and `memcard_freq_load` for the
 * container. It writes `+0x8c` and `+0x90`, which are the MemcardUser and ListDataProvider vptrs,
 * then `+0xb4`, `+0xb8`, `+0xbc`, `+0xd4`, `+0xd8`, a vector, and `+0xe4`.
 *
 * It pushes the object name `del_freq` into the container object-name vector that MetScreen owns.
 * It emits two secondary vtables and none for MetMemCardPickerUser, and writes no vptr at `+0x94`
 * where that subobject sits, which is one of the five observations that prove the class declares
 * no virtual function.
 *
 * The object is at least 0xe8 bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor is at `0x002beca8`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x002c5be8`, 7 `0x002bf750`, 15 `0x002c0ad8`, 19 `0x002bf3a8`, 20 `0x002c5b58`, 23
 * `0x002c5b48`, 24 `0x002c5b50`, 36 `0x002c01c0`, 38 `0x002bee90`.
 */
class MetMCFreqDelScreen :
    public MetScreen,
    public MemcardUser,
    public ListDataProvider,
    public MetMemCardPickerUser {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002be968
     */
    MetMCFreqDelScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x002beca8
     */
    virtual ~MetMCFreqDelScreen();
};
