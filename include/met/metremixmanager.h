#pragma once

#include "memcard/memcarduser.h"
#include "met/metscreen.h"
#include "os/asynccallback.h"

/**
 * Owner of the remix catalogue, which also presents itself as a dialogue screen.
 *
 * `15MetRemixManager` in the RTTI descriptor at `0x008efc50`, with three public non-virtual bases
 * at fixed offsets, MetScreen at `+0x00`, MemcardUser at `+140`, and AsyncCallback at `+144`. Its
 * own file is `MetRemixManager.cpp`, which its asserts record at `0x00807bb0`, and it is one of
 * only two classes in the subsystem whose file name survives in the image.
 *
 * Three vtables belong to the class, the 39-entry primary at `0x00807de0`, the 21-entry
 * MemcardUser table at `0x00807d30` that adjusts `this` by `-140`, and the three-entry
 * AsyncCallback table at `0x00807d10` that adjusts it by `-144`. The primary is the same length as
 * the MetScreen table, so the class declares no virtual of its own.
 *
 * This declaration is deliberately partial. The constructor at `0x00352b80` runs for several
 * hundred instructions. It supplies `dlg` for the screen name, `metagame/Shared` for the
 * directory, and `dialogue` for the container, writes the three vptrs, zeroes `+0x94`, and then
 * builds two red-black tree headers, at `+0x98` and `+0xa0`, each by taking a 16-byte node from
 * the pool through StlAllocatePoolBlock and self-linking it. Three vectors follow at `+0xac`,
 * `+0xb8`, and `+0xc4`, and the routine goes on to write `+0xd4` through `+0xf4` and `+0x13c`
 * among others. The element types of the two trees and the three vectors are not recovered, the
 * span past `+0xd0` is not modelled, and the total size is not recovered, so everything from
 * `+0x98` onward is recorded rather than declared.
 *
 * The destructor is at `0x00355070`.
 *
 * Five slots differ from the MetScreen table, and none has a recovered name. They are
 * 5 `0x00361518`, 15 `0x003555c0`, and 36 `0x00361550`.
 */
class MetRemixManager : public MetScreen, public MemcardUser, public AsyncCallback {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00352b80
     */
    MetRemixManager(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x00355070
     */
    virtual ~MetRemixManager();

private:
    int mUnknown94; // +0x94
};
