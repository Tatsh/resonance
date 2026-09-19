#pragma once

#include <vector>

#include "game/idablebase.h"

/**
 * Mix-in that registers its most derived object in a per-type identifier table.
 *
 * `IDable<T>` in the RTTI, mangled `t6IDable1Z6Player` for the one instantiation the image holds.
 * The descriptor at `0x008ef1f0` records `IDableBase` as its only base at offset 0, so the
 * subobject adds no data of its own and reuses mId at `+0x00` with its own vptr at `+0x04`.
 *
 * Each instantiation has one table of object pointers. For `IDable<Player>` that table is the
 * vector at `0x0066f920`, which the static-initialisation stub at `0x00132618` empties before any
 * player exists.
 *
 * Three words decide the type. The stub zeroes `0x0066f920`, `0x0066f924`, and `0x0066f928` in
 * sequence, and `0x0066f924` takes three writes from that stub and no read anywhere else, which is
 * a begin, end, and capacity triple rather than a bare pointer. The destructor indexing the table
 * by loading the first word and adding four times mId is equally consistent with either reading,
 * so it does not settle anything on its own.
 *
 * Modelling the table as a static member of the template rather than as a file-scope global is an
 * inference from the one-per-instantiation shape. Four Player destructors and eight unrelated
 * readers index the same vector, and a protected static member and a global fit both equally
 * well.
 */
template <typename T>
class IDable : public IDableBase {
public:
    /**
     * Release the table slot this object occupies.
     *
     * An object that never registered retains kIDableUnregistered and skips the clear.
     *
     * @ghidraAddress 0x00121db0
     */
    virtual ~IDable();

protected:
    // Indexed by mId. A cleared slot is a null pointer rather than a removed element, so the
    // vector never shrinks.
    static std::vector<T *> sObjects;
};

template <typename T>
std::vector<T *> IDable<T>::sObjects;

// 0x00121db0
template <typename T>
IDable<T>::~IDable() {
    if (mId != kIDableUnregistered) {
        sObjects[mId] = nullptr;
    }
}
