#pragma once

#include <cstddef>

#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/collideable.h"
#include "rnd/drawable.h"
#include "rnd/transformable.h"

class FailSink;
namespace Rnd {
class Object;
class Stream;
} // namespace Rnd

namespace Rnd {

/**
 * Root of a loadable scene.
 *
 * `Q23Rnd4View` in the RTTI descriptor at `0x008ef120`. The descriptor lists four public
 * non-virtual bases and fixes their subobject offsets, `Rnd::Animatable` at `+0x00`,
 * `Rnd::Drawable` at `+0x18`, `Rnd::Transformable` at `+0x30`, and `Rnd::Collideable` at `+0xe0`.
 * The shared `Rnd::Object` virtual base sits at `+0x100`, and the constructor writes that address
 * into the virtual-base pointer of all four subobjects. View's own four fields occupy `+0xf0`
 * through `+0xff` and start zeroed.
 *
 * Five vtables belong to the class, one per subobject plus the one the `Rnd::Object` subobject
 * vptr addresses. They are `0x00823628` (Animatable), `0x00823600` (Drawable), `0x008235e0`
 * (Transformable), `0x008235c0` (Collideable), and `0x00823650` (Object). Every entry of the last
 * four adjusts `this` back to the start of the View. The Drawable table repeats the base
 * implementations of SetShowing, SetHighlight, and DrawSelf, so View overrides none of the three
 * and draws nothing of its own.
 *
 * A `.rnd` file exposes its scene root under the name "view", which start-up resolves through
 * Rnd::Manager::Find().
 *
 * Five class keys resolve to this one class, which is what the four flags below record. Init()
 * registers "View" against a factory that sets no flag, and "Animatable", "Collideable",
 * "Drawable", and "Transformable" against four factories that each set one. A file naming a bare
 * mix-in therefore loads a View that remembers which mix-in was asked for.
 *
 * The destructor follows the engine pattern of dropping this object's own references and then
 * ReleaseAllRefs(). A view holds no reference of its own, so the first step is an empty body.
 */
class View : public Animatable, public Drawable, public Transformable, public Collideable {
public:
    /**
     * Allocate a view from the tagged heap under the tag "Rnd::View".
     *
     * @param nSize The object size, which the compiler supplies.
     * @return The block.
     * @ghidraAddress 0x004e2048
     */
    void *operator new(size_t nSize);

    /**
     * Release a view to the tagged heap.
     *
     * @param pBlock The block.
     * @ghidraAddress 0x004e2068
     */
    void operator delete(void *pBlock);

    /**
     * Construct an empty scene root.
     *
     * The four flags below start zeroed, and each of the four base subobjects is constructed
     * against the one shared `Rnd::Object` subobject at `+0x100`.
     *
     * @param name The registry key for this object.
     * @ghidraAddress 0x004e2740
     */
    explicit View(const HxStr &name);

    /** @ghidraAddress 0x004e21b8 */
    virtual ~View();

    /** @ghidraAddress 0x004e3768 */
    virtual void DumpText(FailSink &sink);

    /** @ghidraAddress 0x004e37d0 */
    virtual void Save(Stream &stream);

    /** @ghidraAddress 0x004e36f8 */
    virtual void Replace(Object *pFrom, Object *pTo);

    /** @ghidraAddress 0x004e2720 */
    virtual const HxStr &ClassName() const;

    /** @ghidraAddress 0x004e3850 */
    virtual void Copy(const Object *pSource, unsigned nFlags);

    /** @ghidraAddress 0x004e0128 */
    virtual void Load(Stream &stream);

    /**
     * Build a view the class registry vends.
     *
     * @param name The registry key for the new view.
     * @return The new view.
     * @ghidraAddress 0x004e2088
     */
    static View *NewView(const HxStr &name);

    /**
     * Register the five class keys this class answers to with Rnd::g_manager.
     *
     * The four mix-in keys are registered from temporary strings the routine builds from literals,
     * and only the "View" key uses a string global.
     *
     * @ghidraAddress 0x004dff48
     */
    static void Init();

    /**
     * Add another view as a child in all three roles.
     *
     * Appends it to the draw list and adds it to the animation and transform lists. The body is
     * defined in the unit of the tunnel object cache, and Overlay's constructor is the one caller.
     * The title is inferred.
     *
     * @param pChild The view to add.
     * @ghidraAddress 0x0040f660
     */
    void AddView(View *pChild);

    /**
     * Remove another view from all four child roles.
     *
     * Removes it from the draw, animation, transform, and collision lists. The body is defined in
     * the unit of the tunnel object cache, and Overlay's constructor is the one caller. The title
     * is inferred.
     *
     * @param pChild The view to remove.
     * @ghidraAddress 0x0040f5f8
     */
    void RemoveView(View *pChild);

private:
    // Drop the references this view holds, of which there are none, so the body is empty. The
    // destructor calls it immediately before ReleaseAllRefs(), where every sibling class drops its
    // own references. The name follows that pattern and is inferred. 0x004e2730.
    void RemoveObjectRefs();

public:
    // Declared in recovered offset order. Each flag is titled from the class key of the factory
    // that sets it. Every writer is one of those factories and no reader was located, so the four
    // are public because nothing in the image constrains them further.

    int mAnimatable;    /*!< Set when the class key was "Animatable". +0xf0 */
    int mTransformable; /*!< Set when the class key was "Transformable". +0xf4 */
    int mDrawable;      /*!< Set when the class key was "Drawable". +0xf8 */
    int mCollideable;   /*!< Set when the class key was "Collideable". +0xfc */
};

/**
 * Class key a `.rnd` file writes for a view.
 *
 * @ghidraAddress 0x00702b20
 */
extern HxStr g_viewClassName;

} // namespace Rnd
