#pragma once

#include "game/idablebase.h"

/**
 * Mix-in that registers its most derived object in a per-type identifier table.
 *
 * `IDable<T>` in the RTTI, mangled `t6IDable1Z6Player` for the one instantiation the image holds.
 * The descriptor at `0x008ef1f0` records `IDableBase` as its only base at offset 0, so the
 * subobject adds no data of its own and reuses mId at `+0x00` with its own vptr at `+0x04`.
 *
 * Each instantiation owns one table of object pointers, reached through a global. For
 * `IDable<Player>` that global is at `0x0066f920`. The destructor clears this object's slot by
 * indexing the table with mId, and skips the clear when mId is kIDableUnregistered, which is the
 * value an object that never registered retains.
 */
template <typename T>
class IDable : public IDableBase {
public:
    /** @ghidraAddress 0x00121db0 */
    virtual ~IDable();
};
