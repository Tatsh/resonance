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
 * Each instantiation has one table of object pointers. For `IDable<Player>` the table is the vector
 * at `0x0066f920`. The static-initialisation stub at `0x00132618` fills the vector with four null
 * pointers before any player exists.
 *
 * Three words decide the type. The stub zeroes `0x0066f920`, `0x0066f924`, and `0x0066f928` in
 * sequence, and `0x0066f924` takes three writes from that stub and no read anywhere else, which is
 * a begin, end, and capacity triple rather than a bare pointer. The destructor indexing the table
 * by loading the first word and adding four times mId is equally consistent with either reading,
 * so it does not settle anything on its own.
 *
 * Modelling the table as a static member of the template rather than as a file-scope global is an
 * inference from the one-per-instantiation shape. Four Player destructors and eight unrelated
 * readers index the same vector, and a public static member and a global fit both equally well.
 */
template <typename T>
class IDable : public IDableBase {
public:
    /**
     * Register the object in sObjects under an identifier.
     *
     * Inline. Player's constructor at `0x0012f5c0` expands it, storing the identifier and then,
     * unless it is kIDableUnregistered, the object into the slot the identifier indexes. The slot
     * must already exist, because the vector is not grown.
     *
     * @param nId The identifier.
     */
    explicit IDable(int nId) {
        mId = nId;
        if (nId != kIDableUnregistered) {
            sObjects[nId] = static_cast<T *>(this);
        }
    }

    /**
     * Release the table slot this object occupies.
     *
     * An object that never registered retains kIDableUnregistered and skips the clear.
     *
     * @ghidraAddress NTSC-U/C: 0x00121db0
     * @ghidraAddress PAL: 0x001223b8
     */
    virtual ~IDable();

    /**
     * Every registered object, indexed by mId.
     *
     * A cleared slot is a null pointer rather than a removed element, and the vector never shrinks.
     * Public because readers outside the hierarchy index it directly, among them Phrase::Load() at
     * `0x001b540c`, and the image exposes no accessor.
     */
    static std::vector<T *> sObjects;
};

/**
 * Slots each identifier table starts with, one per player. The static initialiser at `0x00132618`
 * allocates four null pointers and the table never grows. Identifiers 0 to 3 are the only ones a
 * constructor can register.
 */
constexpr int kIDableInitialSlots = 4;

template <typename T>
std::vector<T *> IDable<T>::sObjects(kIDableInitialSlots);

// NTSC-U/C: 0x00121db0, PAL: 0x001223b8
template <typename T>
IDable<T>::~IDable() {
    if (mId != kIDableUnregistered) {
        sObjects[mId] = nullptr;
    }
}
