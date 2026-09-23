#pragma once

#include <list>

#include "rnd/object.h"

class FailSink;
class ScrollingList;
namespace Rnd {
class Stream;
}

namespace Rnd {

/**
 * Line segment a collision query is run along.
 *
 * The title is inferred; the type is a plain record with no RTTI and no allocation tag of its own.
 * It is 0x20 bytes, two 16-byte vectors whose fourth float is padding. The ray and sphere test at
 * `0x005501b0` proves the layout by subtracting `+0x00` from `+0x10` to obtain the direction.
 */
struct Ray {
    float mStart[4]; /*!< Origin. The fourth float is padding. +0x00 */
    float mEnd[4];   /*!< Far end. The fourth float is padding. +0x10 */
};

/**
 * Mix-in for an object that can be hit-tested.
 *
 * `Q23Rnd11Collideable` in the RTTI descriptor at `0x008ef440`, with `Rnd::Object` as a public
 * virtual base at offset 0. The subobject is 0xc bytes. The compiler places the virtual-base
 * pointer at `+0x00` and, following the g++ 2.x layout for a class with no non-virtual base, the
 * vptr at `+0x08`. The one declared member therefore sits at `+0x04`. For a standalone Collideable
 * the `Rnd::Object` subobject sits at `+0x0c`. The `-0xc` adjustment on every entry of the second
 * vtable confirms that placement.
 *
 * Two vtables belong to the class. The three-entry table at `0x008250d8` is addressed by the vptr
 * at `+0x08` and stores the two virtuals declared here. The nine-entry table at `0x008250f8` is
 * addressed by the `Rnd::Object` subobject vptr and stores the overrides of the `Rnd::Object`
 * virtuals, each with a `-0xc` adjustment back to the Collideable subobject. Slot 5 of the second
 * table still addresses the pure-virtual stub at `0x005381a8`. ClassName() is therefore
 * unimplemented here and the class remains abstract.
 *
 * Every entry of mCollides registers this object as a referrer through Rnd::Object::AddRef(). A
 * collideable that goes away is therefore removed from its parents' lists by Replace() rather than
 * remaining as a stale pointer. The arrangement is the collision counterpart of Rnd::Drawable and
 * its mDraws list.
 */
class Collideable : public virtual Object {
    // ScrollingList's destructor walks mCollides directly, and the image has no accessor for it.
    friend class ::ScrollingList;

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
     * `Rnd::Mesh::Collide` appends to through the generic `stl_list` allocation tag. Nothing
     * identifies the type. Whether the collector has further fields is not recovered.
     */
    struct HitSink {
        std::list<Hit> mHits; // +0x00
    };

    /**
     * Construct a collideable with no children.
     *
     * The constructor writes the two vtable pointers and allocates the mCollides sentinel. It
     * therefore has no written body of its own.
     *
     * @ghidraAddress 0x00502668
     */
    Collideable();

    /**
     * Drop this object's references on its children.
     *
     * @ghidraAddress 0x00502518
     */
    virtual ~Collideable();

    /**
     * Report the collideable that hit-tests this one.
     *
     * Walks the referrer list of the `Rnd::Object` subobject, casts each referrer to Collideable
     * and returns the first whose own mCollides list includes this object. The result is therefore
     * the parent in the collision hierarchy rather than a plain cast.
     *
     * @return The parent collideable, or null when no referrer hit-tests this one.
     * @ghidraAddress 0x00500348
     */
    Collideable *Parent();

    /**
     * Append pCollide to mCollides.
     *
     * Registers this object as a referrer of pCollide. A pCollide already in mCollides produces the
     * report "%s already in %s" and no insertion. `Rnd::Manager::Read` invokes this while linking a
     * loaded scene.
     *
     * @param pCollide The collideable to add.
     * @ghidraAddress 0x00500858
     */
    void AddCollide(Collideable *pCollide);

    /**
     * Erase pCollide from mCollides.
     *
     * Drops this object's reference on pCollide first. A pCollide absent from mCollides does
     * nothing.
     *
     * @param pCollide The collideable to remove.
     * @ghidraAddress 0x005009d8
     */
    void RemoveCollide(Collideable *pCollide);

    /**
     * Test a ray against this object and append what it strikes to sink.
     *
     * Vtable slot 1. The base implementation tests nothing of its own and forwards the pair to
     * every mCollides entry. `Rnd::Mesh::Collide` at `0x0047f950` ends by chaining here so that its
     * children are tested after its own faces.
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
     * implementation. Only the overrides can distinguish the two. Slot 1 is overridden by
     * `Rnd::Mesh`, `Rnd::Tunnel`, and `Rnd::Arena`. Every derived table examined so far stores this
     * base implementation in slot 2, with one exception. Rnd::Cam overrides it at `0x004ad820`
     * with a routine that tests the ray start point against its screen rectangle, which makes
     * the second query a screen-space pick rather than a geometric one.
     *
     * @param ray The segment to test along.
     * @param sink The collector to append intersections to.
     * @ghidraAddress 0x00502ab8
     */
    virtual void CollideUnknown(const Ray &ray, HitSink &sink);

    /**
     * Write a description of this object to sink.
     *
     * Writes the mCollides list, and produces nothing at all when the dump level of sink is not
     * positive. FailSink::Print() discards its text in the shipped build. The routine therefore
     * produces no output on this target in any case.
     *
     * @param sink The diagnostic sink to write to.
     * @ghidraAddress 0x00502880
     */
    virtual void DumpText(FailSink &sink);

    /**
     * Write the revision and the mCollides list to stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x005028f0
     */
    virtual void Save(Stream &stream);

    /**
     * Retarget every mCollides entry equal to pFrom at pTo.
     *
     * @param pFrom The object being replaced.
     * @param pTo The replacement, or null.
     * @ghidraAddress 0x00500410
     */
    virtual void Replace(Object *pFrom, Object *pTo);

    /**
     * Copy the child list of pSource when nFlags requests it.
     *
     * Unlike Rnd::Drawable::Copy(), this routine reads no scalar field of the source. The class
     * declares none.
     *
     * @param pSource The object to copy from.
     * @param nFlags The set of fields to copy; see kCopyChildLists.
     * @ghidraAddress 0x00500730
     */
    virtual void Copy(const Object *pSource, unsigned nFlags);

    /**
     * Replace the child list from stream.
     *
     * A revision above the one this build writes produces the report "Can't load new Collideable"
     * followed by the abort handler of g_failSink.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x005005f0
     */
    virtual void Load(Stream &stream);

protected:
    /**
     * Drop this object's reference on every mCollides entry without emptying the list.
     *
     * Every derived destructor invokes this before its own teardown, `Rnd::Mesh` and `Rnd::View`
     * among them.
     *
     * @ghidraAddress 0x00502948
     */
    void ReleaseCollidesRefs();

private:
    // 0x005029b8
    // Only Copy() and Load() invoke this, and both inline it.
    void AcquireCollidesRefs();

    // The collideables this one hit-tests after itself, each of which registers this object as a
    // referrer.
    std::list<Collideable *> mCollides; // +0x04
};

} // namespace Rnd
