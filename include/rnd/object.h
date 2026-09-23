#pragma once

#include <list>

#include "os/hxstr.h"

class FailSink;
namespace Rnd {
class Stream;
}

namespace Rnd {

/**
 * Bit of the Copy() flags word that requests copying the child object lists.
 *
 * Recovered from `Rnd::Drawable::Copy()` and `Rnd::Animatable::Copy()`, both of which copy their
 * child list only when this bit is set.
 */
constexpr unsigned kCopyChildLists = 0x200;

/**
 * Base of every object the renderer can load, resolve by name, and serialise.
 *
 * `Q23Rnd6Object` in the RTTI descriptor at `0x0086f678`, a leaf class with no base. Every
 * renderer mix-in (Animatable, Collideable, Drawable, Transformable) derives from this class
 * virtually, which places the subobject at the end of the most derived object.
 *
 * The class is 0x1c bytes. Following the g++ 2.x layout for a class with no base, the vptr sits
 * after the data members at `+0x18`, and the eight-entry vtable is at `0x00829010`.
 *
 * Construction registers the object in Rnd::g_manager under its name, and destruction erases that
 * registration. A second object stores a pointer to this one by registering itself through
 * AddRef(). ReleaseAllRefs() then notifies every referrer through Replace() before this object
 * goes away.
 */
class Object {
public:
    /**
     * Construct an unnamed object.
     *
     * mInternal, mMerge, and mDeleting are not written, so an object built this way starts with
     * three indeterminate fields. The named constructor does initialise all three.
     *
     * @ghidraAddress 0x0053fc08
     */
    Object();

    /**
     * Construct an object and register it under name.
     *
     * A name already present in Rnd::g_manager produces the report "%s already exists" and then
     * transfers control to the abort handler of g_failSink. The map entry is overwritten with this
     * object either way.
     *
     * @param name The registry key for this object.
     * @ghidraAddress 0x0053e0d8
     */
    explicit Object(const HxStr &name);

    /**
     * Erase the registry entry and release the name buffer.
     *
     * @ghidraAddress 0x0053e348
     */
    virtual ~Object();

    /**
     * Re-register this object under a different name.
     *
     * A name equal to mName is discarded without further work, and a name already present in
     * Rnd::g_manager produces the report "%s already exists" and no change.
     *
     * @param name The new registry key.
     * @ghidraAddress 0x0053e400
     */
    void SetName(const HxStr &name);

    /**
     * Register pReferrer as a store of a pointer to this object.
     *
     * A pReferrer equal to this object is discarded. The same referrer may register more than
     * once, and ReleaseAllRefs() collapses the duplicates.
     *
     * @param pReferrer The object that stores a pointer to this one.
     * @ghidraAddress 0x0053e720
     */
    void AddRef(Object *pReferrer);

    /**
     * Drop one registration made by AddRef().
     *
     * Does nothing while mDeleting is set, because ReleaseAllRefs() is already walking the list.
     *
     * @param pReferrer The object whose registration is to be dropped.
     * @ghidraAddress 0x0053e7d0
     */
    void RemoveRef(Object *pReferrer);

    /**
     * Write a human-readable description of this object to sink.
     *
     * Vtable slot 2. The referrer list is included only when the dump level of sink is positive.
     * FailSink::Print() is two instructions that discard their text in the shipped build, so this
     * routine produces no output on this target. The literals it passes survive in `.rodata`
     * regardless, which is where the recovered member titles came from.
     *
     * @param sink The diagnostic sink to write to.
     * @ghidraAddress 0x0053e5a8
     */
    virtual void DumpText(FailSink &sink);

    /**
     * Write this object's serialised form to stream.
     *
     * Vtable slot 3.
     *
     * @param stream The stream to write to.
     */
    virtual void Save(Stream &stream) = 0;

    /**
     * Replace every stored pointer to pFrom with pTo.
     *
     * Vtable slot 4. A null pTo means the referenced object is going away and the pointer is to be
     * dropped.
     *
     * @param pFrom The object being replaced.
     * @param pTo The replacement, or null.
     */
    virtual void Replace(Object *pFrom, Object *pTo) = 0;

    /**
     * Report the name of this object's most derived class.
     *
     * Vtable slot 5. The returned string is the key a `.rnd` file writes for the class, not the
     * mangled RTTI name.
     *
     * @return The class name.
     */
    virtual const HxStr &ClassName() const = 0;

    /**
     * Copy the state of pSource into this object.
     *
     * Vtable slot 6. Only the fields selected by nFlags are copied; see kCopyChildLists.
     *
     * @param pSource The object to copy from.
     * @param nFlags The set of fields to copy.
     */
    virtual void Copy(const Object *pSource, unsigned nFlags) = 0;

    /**
     * Read this object's serialised form from stream.
     *
     * Vtable slot 7.
     *
     * @param stream The stream to read from.
     */
    virtual void Load(Stream &stream) = 0;

protected:
    /**
     * Tell every referrer to drop its pointer to this object.
     *
     * Sets mDeleting, collapses adjacent duplicates in mRefs, calls Replace() on each referrer
     * with a null replacement, then empties mRefs. Every derived destructor runs this immediately
     * before the object goes away.
     *
     * @ghidraAddress 0x0053fca0
     */
    void ReleaseAllRefs();

    // Declared in recovered offset order, with the access specifiers interleaved.

    // Objects that store a pointer to this one. Rnd::Drawable::Parent() and its siblings walk it.
    std::list<Object *> mRefs; // +0x00

public:
    HxStr mName; /*!< Registry key. Public because the draws-list writer and the reference dumper
                      both read the name of an unrelated object, and the image exposes no accessor
                      for it; either call could equally be an inlined accessor. +0x04 */

    int mInternal; /*!< Set for an object the renderer created rather than a file. Public because
                        Rnd::Manager::Read() tests it at `0x0051b6e4` when it finds an existing
                        object under an incoming name, and ClearInternalObjects() sweeps on it.
                        Both are outside this hierarchy and the image has no accessor. +0x0c */

private:
    // Rnd::Manager::Read() sets mMerge and Rnd::Manager::Write() saves it.
    friend class Manager;

    int mMerge; // +0x10
    // Set for the whole of ReleaseAllRefs() so that a referrer notified through Replace() cannot
    // erase an mRefs entry the walk is still standing on.
    int mDeleting; // +0x14
};

/**
 * Hook invoked with the object whenever a registry key is about to change.
 *
 * Both the destructor and SetName() call this before erasing the old key. The hook is optional and
 * starts null.
 *
 * @ghidraAddress 0x00719860
 */
extern void (*g_pfnNameChanged)(Object *pObject);

} // namespace Rnd
