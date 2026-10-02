#pragma once

#include <list>

#include "rnd/animatable.h"
#include "rnd/collideable.h"
#include "rnd/drawable.h"
#include "rnd/object.h"
#include "rnd/transformable.h"

namespace Rnd {

// Each overload appends every descendant along one relation, depth first, each object before its
// own descendants. A null start appends nothing. The four are defined in this header, and every
// unit that uses them emits its own copies: Rnd::Manager's at `0x00519fc8`, `0x0051a0e0`,
// `0x0051a1f8`, and `0x0051a310`, and ScrollingList's at `0x003fcf20`, `0x003fd038`,
// `0x003fd150`, and `0x003fd268`.

/**
 * Append every animation descendant of an animatable.
 *
 * @param objects The list to append to.
 * @param pAnimatable The animatable to start from, or null.
 * @ghidraAddress 0x00519fc8
 */
inline void CollectChildren(std::list<Object *> &objects, Animatable *pAnimatable) {
    if (pAnimatable == nullptr) {
        return;
    }
    for (std::list<Animatable *>::iterator it = pAnimatable->mAnims.begin();
         it != pAnimatable->mAnims.end();
         ++it) {
        objects.push_back(*it);
        CollectChildren(objects, *it);
    }
}

/**
 * Append every collision descendant of a collideable.
 *
 * @param objects The list to append to.
 * @param pCollideable The collideable to start from, or null.
 * @ghidraAddress 0x0051a0e0
 */
inline void CollectChildren(std::list<Object *> &objects, Collideable *pCollideable) {
    if (pCollideable == nullptr) {
        return;
    }
    for (std::list<Collideable *>::iterator it = pCollideable->mCollides.begin();
         it != pCollideable->mCollides.end();
         ++it) {
        objects.push_back(*it);
        CollectChildren(objects, *it);
    }
}

/**
 * Append every draw descendant of a drawable.
 *
 * @param objects The list to append to.
 * @param pDrawable The drawable to start from, or null.
 * @ghidraAddress 0x0051a1f8
 */
inline void CollectChildren(std::list<Object *> &objects, Drawable *pDrawable) {
    if (pDrawable == nullptr) {
        return;
    }
    std::list<Drawable *> &draws = pDrawable->GetDraws();
    for (std::list<Drawable *>::iterator it = draws.begin(); it != draws.end(); ++it) {
        objects.push_back(*it);
        CollectChildren(objects, *it);
    }
}

/**
 * Append every transform descendant of a transformable.
 *
 * @param objects The list to append to.
 * @param pTransformable The transformable to start from, or null.
 * @ghidraAddress 0x0051a310
 */
inline void CollectChildren(std::list<Object *> &objects, Transformable *pTransformable) {
    if (pTransformable == nullptr) {
        return;
    }
    for (std::list<Transformable *>::iterator it = pTransformable->mTransList.begin();
         it != pTransformable->mTransList.end();
         ++it) {
        objects.push_back(*it);
        CollectChildren(objects, *it);
    }
}

} // namespace Rnd
