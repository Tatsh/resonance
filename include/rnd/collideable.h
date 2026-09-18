#pragma once

#include <list>

#include "rnd/object.h"

namespace Rnd {

/**
 * Line segment a collision query is run along.
 *
 * The title is inferred; the type is a plain record with no RTTI and no allocation tag of its own.
 * It is 0x20 bytes, two 16-byte vectors whose fourth float is padding, which the ray and sphere
 * test at `0x005501b0` proves by subtracting `+0x00` from `+0x10` to obtain the direction.
 */
struct Ray {
    float mStart[4]; /*!< Origin. The fourth float is padding. +0x00 */
    float mEnd[4];   /*!< Far end. The fourth float is padding. +0x10 */
};

/**
 * Mix-in for an object that can be hit-tested.
 *
 * `Q23Rnd11Collideable` in the RTTI descriptor at `0x008ef440`, with `Rnd::Object` as a public
 * virtual base at offset 0. The subobject is 0xc bytes: the virtual-base pointer at `+0x00`,
 * mCollides at `+0x04`, and the vptr at `+0x08`. `Rnd::View` confirms the vptr offset by placing
 * its Collideable subobject at `+0xe0` and writing that vptr to View + 0xe8.
 *
 * The class declares two virtuals of its own beyond the compiler-generated slot 0. Both take a
 * ray and a collector and both base implementations do nothing but forward the pair to the same
 * slot on every mCollides entry, which is the collision counterpart of how
 * `Rnd::Drawable::Draw()` walks mDraws. `Rnd::Mesh::Collide` at `0x0047f950` ends by chaining to
 * the slot 1 base implementation so that its children are tested after its own faces.
 *
 * Its remaining members are the constructor at `0x00502668`, the destructor at `0x00502518`,
 * DumpText at `0x00502880`, Save at `0x005028f0`, and the list teardown at `0x00502948`, none of
 * which is reconstructed yet.
 */
class Collideable : public virtual Object {
public:
    /**
     * One recorded intersection.
     *
     * The title is inferred. Only the two fields `Rnd::Mesh::Collide` fills are recovered, from
     * the record it builds at `0x0047fd74` after a successful face test. It is nested because it
     * refers back to the enclosing class, the way `Rnd::Animatable::Filter` is nested.
     */
    struct Hit {
        Collideable *mObject; /*!< The collideable that was struck. +0x00 */
        float mDistance;      /*!< Distance along the ray. +0x04 */
    };

    /**
     * Collector a collision query appends its intersections to.
     *
     * The title is inferred. The only recovered field is the list at `+0x00`, which
     * `Rnd::Mesh::Collide` appends to through the generic `stl_list` allocation tag, so nothing
     * names the type. Whether the collector has further fields is not recovered.
     */
    struct HitSink {
        std::list<Hit> mHits; // +0x00
    };

    virtual ~Collideable();

    /**
     * Report the collideable that hit-tests this one.
     *
     * Walks the referrer list of the `Rnd::Object` subobject, casts each referrer to Collideable
     * and returns the first whose own mCollides list includes this object, which makes the result
     * the parent in the collision hierarchy rather than a plain cast.
     *
     * @return The parent collideable, or null when no referrer hit-tests this one.
     * @ghidraAddress 0x00500348
     */
    Collideable *Parent();

    /**
     * Test a ray against this object and append what it strikes to sink.
     *
     * Vtable slot 1. The base implementation tests nothing of its own and forwards the pair to
     * every mCollides entry.
     *
     * @param ray The segment to test along.
     * @param sink The collector to append intersections to.
     * @ghidraAddress 0x00502a28
     */
    virtual void Collide(const Ray &ray, HitSink &sink);

    /**
     * Second collision query, whose purpose is unrecovered.
     *
     * Vtable slot 2. It has the same signature as Collide() and the same forwarding base
     * implementation, so only the overrides can distinguish the two.
     *
     * @param ray The segment to test along.
     * @param sink The collector to append intersections to.
     * @ghidraAddress 0x00502ab8
     */
    virtual void CollideUnknown(const Ray &ray, HitSink &sink);

private:
    std::list<Collideable *> mCollides; // +0x04
};

} // namespace Rnd
