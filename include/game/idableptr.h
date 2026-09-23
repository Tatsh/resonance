#pragma once

#include "game/idable.h"
#include "game/idablebase.h"
#include "game/nullplayer.h"

/**
 * Reference to a registered object by its identifier, resolved on first use.
 *
 * The name is inferred. No descriptor, allocation tag, or literal in the image identifies the
 * type, and the class is never emitted out of line. Its layout and behaviour come from the packets
 * that embed it: BumpPacket, CripplePacket, CaughtPhrasePacket, and CatchProgressPacket at
 * `+0x14`, and TrackSelectPacket at `+0x18`.
 *
 * The object is eight bytes, a cached pointer and an identifier. Every New() of those packets
 * stores 0 and -1 into the pair. Save() and Load() transfer only the identifier, and Load() writes
 * it in place without clearing the cached pointer. Print() then resolves the pair with the same
 * inline sequence at every site, `0x003e7c64` in CripplePacket::Print() and `0x003f259c` in
 * TrackSelectPacket::Print() among them. A null cache takes the stand-in object when the
 * identifier is kIDableUnregistered and the table entry otherwise, and the result is cached.
 *
 * The one instantiation is for Player, whose stand-in is g_nullPlayer. The resolution names that
 * object directly, so the template is written for that instantiation alone.
 *
 * Both members are public because the packets' transfer members read and write the identifier
 * directly and the image exposes no accessor.
 */
template <typename T>
class IDablePtr {
public:
    /** Start unresolved, with no identifier. */
    IDablePtr() : mCached(nullptr), mId(-1) {
    }

    /**
     * Refer to an object, already resolved.
     *
     * Inline, with no address of its own. PhraseMgr::SetPhraseOwner() at `0x001bafa8` expands it
     * for a CaughtPhrasePacket on its stack, caching the object and copying its identifier, or -1
     * for a null object.
     *
     * @param pObject The object, or null.
     */
    explicit IDablePtr(T *pObject) : mCached(pObject), mId(pObject != nullptr ? pObject->mId : -1) {
    }

    /**
     * Resolve the identifier to an object, caching the result.
     *
     * An identifier of -1 is not tested for, so an unresolved pair with no identifier indexes the
     * table at -1. That matches every inline copy in the image.
     *
     * @return The object.
     */
    operator T *() {
        if (mCached == nullptr) {
            mCached = mId == kIDableUnregistered ? &g_nullPlayer : IDable<T>::sObjects[mId];
        }
        return mCached;
    }

    T *mCached; /*!< The resolved object, or null before the first resolution. +0x00 */
    int mId;    /*!< The object's identifier in IDable<T>::sObjects. +0x04 */
};
