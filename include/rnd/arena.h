#pragma once

#include <list>
#include <vector>

#include "math/vector3.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/collideable.h"
#include "rnd/drawable.h"
#include "rnd/raytest.h"
#include "rnd/transformable.h"

class FailSink;
namespace Rnd {
class Object;
class Stream;
class View;
} // namespace Rnd

namespace Rnd {

/**
 * Loop of views the renderer walks as one continuous space.
 *
 * `Q23Rnd5Arena` in the RTTI descriptor at `0x008f0440`, whose name string is at `0x008342f8` and
 * whose four base entries record `Rnd::Animatable` at `+0x00`, `Rnd::Collideable` at `+0x18`,
 * `Rnd::Transformable` at `+0x30`, and `Rnd::Drawable` at `+0xe0`, each non-virtual and public.
 * All four derive virtually from `Rnd::Object`, so one shared Object subobject sits at `+0x130`,
 * which the constructor proves by writing `this + 0x130` into the virtual-base pointer of each of
 * the four subobjects. The class is therefore 0x14c bytes and the creator allocates 0x150.
 *
 * The four offsets account for every byte ahead of `+0xf4`. Animatable uses 0x18 and needs no
 * padding, Collideable uses 0xc and the twelve bytes after it are the padding that puts the
 * 16-byte-aligned Transformable subobject on `+0x30`, Transformable uses 0xac and rounds to 0xb0,
 * and Drawable uses 0x14. The twelve bytes from `+0xf4` are the padding that puts the first
 * member, which is 16-byte aligned, on `+0x100`, and the twelve past the last member are the same
 * rounding applied to the non-virtual part before the virtual base subobject is placed.
 * `Rnd::Generator` shows the identical rounding at its own tail.
 *
 * Five vtables belong to the class, each identified by a GetTypeInfo slot addressing `0x005bb800`
 * and by an adjustment matching its subobject offset. The Drawable table at `0x00834220` adjusts
 * by `-0xe0` and overrides DrawSelf(). The Transformable table at `0x00834248` adjusts by `-0x30`
 * and overrides UpdateWorldXfm(). The Collideable table at `0x00834268` adjusts by `-0x18` and
 * overrides Collide(). The Animatable table at `0x00834288` has a zero adjustment and overrides
 * SetFrameSelf(). The Object subobject table at `0x008342b0` adjusts by `-0x130` and stores the
 * seven Object virtuals. Every table ends in an all-zero entry, which is the terminator rather
 * than a null slot. No table gains a slot past the own count of its base. This class therefore
 * declares no virtual of its own.
 *
 * The member and section titles come from the text the dumps write: "[Arena]", "loopDist:",
 * " loopFrames:", and "sections:" from DumpText() at `0x005b5f08`, and "\n\tview:", " frame:",
 * " loop:", "\n\tdelta:", " teleport:", and " sortStart:" from the section dump at `0x005b82b0`.
 *
 * `TnlArena` is a different class. It derives from `MsgSink` and belongs to the game rather than
 * the renderer, and so does `MetArenasScreen`.
 *
 * DrawSelf() at `0x005bc020`, UpdateWorldXfm() at `0x005bbf80`, Collide() at `0x005bc090`,
 * SetFrameSelf() at `0x005bbeb0`, Copy() at `0x005bbbb0`, the destructor at `0x005b6600`, and the
 * constructor at `0x005b6c58` are not reconstructed.
 */
class Arena : public Animatable, public Collideable, public Transformable, public Drawable {
public:
    /**
     * One view in the loop.
     *
     * The record is 0x18 bytes, which the vector stride establishes. Every member is public
     * because the section dump, Replace(), and the two hit-list helpers all access them from
     * outside. The record has no behaviour beyond its default constructor.
     */
    struct Section {
        /**
         * Construct a section with no view.
         *
         * Both flags start set, which the temporary the vector reader resizes with proves.
         */
        Section()
            : mView(nullptr), mFrame(0.0f), mDelta(0.0f), mTeleport(1), mSortStart(1), mLoop(0) {
        }

        View *mView;    /*!< View this section draws. +0x00 */
        float mFrame;   /*!< Frame the view is animated to. +0x04 */
        float mDelta;   /*!< Frame step applied as the loop advances. +0x08 */
        int mTeleport;  /*!< Written as "true" or "false" by the section dump. +0x0c */
        int mSortStart; /*!< Written as "true" or "false" by the section dump. +0x10 */
        int mLoop;      /*!< Written with "%d" by the section dump. +0x14 */
    };

    /**
     * Element of mUnknown120.
     *
     * The image supplies no name for the type or for either member. The record is 8 bytes, which
     * the element size the list allocation passes to the node tagger establishes, and the first
     * word is a pointer that Replace() follows through offset zero. That extra indirection proves
     * the static type of the pointer is a class deriving virtually from Rnd::Object rather than
     * Rnd::Object itself, and Section::mView is the only such pointer the class stores, which is
     * the whole of the evidence for the type below.
     */
    struct Unknown120Entry {
        // The temporary the two hit-list helpers resize with has only its first word written, so
        // the second member starts indeterminate and no routine in the image reads it.
        Unknown120Entry() : mView(nullptr) {
        }

        View *mView;    // +0x00
        int mUnknown04; // +0x04
    };

    /**
     * Construct an empty loop.
     *
     * mLoopDist starts at the zero vector, mLoopFrames at 1000.0, and both containers empty. The
     * body is not reconstructed.
     *
     * @param name The registry key for this object.
     * @ghidraAddress 0x005b6c58
     */
    explicit Arena(const HxStr &name);

    /** @ghidraAddress 0x005b6600 */
    virtual ~Arena();

    /**
     * Write the loop to the engine text sink.
     *
     * The four base dumps run unconditionally and the block below them only while the dump level
     * of the sink is positive. The Collideable dump runs last of the four.
     *
     * @param sink The text sink.
     * @ghidraAddress 0x005b5f08
     */
    virtual void DumpText(FailSink &sink);

    /**
     * Write revision 4 of the loop to stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x005b60c0
     */
    virtual void Save(Stream &stream);

    /**
     * Replace one object reference with another.
     *
     * A matching section takes the replacement through a narrowing cast, and a matching entry of
     * mUnknown120 is cleared to null rather than pointed at pTo.
     *
     * @param pFrom The object being replaced.
     * @param pTo The object to point at, which may be null.
     * @ghidraAddress 0x005b6338
     */
    virtual void Replace(Object *pFrom, Object *pTo);

    /**
     * Return the registered class name, "Arena".
     *
     * @return The class name.
     * @ghidraAddress 0x005bb9d8
     */
    virtual const HxStr &ClassName() const;

    /**
     * Copy another loop over this one.
     *
     * The body is not reconstructed.
     *
     * @param pSource The source object, which has to be a loop for the copy to take effect.
     * @param nFlags The copy flags.
     * @ghidraAddress 0x005bbbb0
     */
    virtual void Copy(const Object *pSource, unsigned nFlags);

    /**
     * Read the loop from stream.
     *
     * Rejects revision 5 and above, reporting "Can't load new Arena". The revision word is stored
     * in Rnd::g_nRndArenaLoadRevision rather than in a local.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x005b61e8
     */
    virtual void Load(Stream &stream);

    /**
     * Report the loop displacement.
     *
     * The routine is two instructions that return the address of the member. It is compiled out of
     * line here, so it exists as a function rather than as an accessor this reconstruction added.
     *
     * @return The loop displacement.
     * @ghidraAddress 0x005bb9e8
     */
    Vector3 &LoopDist();

    /**
     * Install the creator hook and register the class with Rnd::g_manager under the key "Arena".
     *
     * @ghidraAddress 0x005bb8b0
     */
    static void Init();

    /**
     * Register this loop as a referrer of every section view and resize mUnknown120.
     *
     * Load() runs it once the sections are in place. mUnknown120 receives one null entry per
     * section.
     *
     * @ghidraAddress 0x005bbc98
     */
    void AddInstancesToHitList();

    /**
     * Drop this loop's registration on every section view and empty mUnknown120.
     *
     * Load() runs it before reading the sections.
     *
     * @ghidraAddress 0x005bbd30
     */
    void RemoveInstancesFromHitList();

protected:
    /**
     * Draw every section.
     *
     * Rnd::Drawable vtable slot 3. The body is not reconstructed.
     *
     * @return Non-zero when the children are to be drawn as well.
     * @ghidraAddress 0x005bc020
     */
    virtual int DrawSelf();

    /**
     * Recompute the world transform.
     *
     * Rnd::Transformable vtable slot 2. The body is not reconstructed.
     *
     * @param pParent The parent whose world transform this one composes with.
     * @param nForce Non-zero to recompute even while the dirty flag is clear.
     * @return Non-zero when the world transform changed.
     * @ghidraAddress 0x005bbf80
     */
    virtual int UpdateWorldXfm(Transformable *pParent, int nForce);

    /**
     * Test a ray against the loop and append what it strikes to sink.
     *
     * Rnd::Collideable vtable slot 1. The body is not reconstructed.
     *
     * @param ray The segment to test along.
     * @param sink The collector to append intersections to.
     * @ghidraAddress 0x005bc090
     */
    virtual void Collide(const Ray &ray, HitSink &sink);

    /**
     * Advance the loop to a frame.
     *
     * Rnd::Animatable vtable slot 3. The body is not reconstructed.
     *
     * @param flFrame The filtered frame to animate to.
     * @ghidraAddress 0x005bbeb0
     */
    virtual void SetFrameSelf(float flFrame);

private:
    // No class derives from Rnd::Arena and nothing outside it accesses a member directly. Every
    // member is therefore private. The order below is the recovered offset order.

    /** Displacement one trip round the loop applies. +0x100 */
    Vector3 mLoopDist;
    float mLoopFrames;              // +0x110
    std::vector<Section> mSections; // +0x114
    // +0x120 A std::list the two hit-list helpers resize to the section count and empty again, and
    // Replace() clears an entry of rather than repointing it. Neither dump writes it and the
    // serialiser does not touch it, so no routine in the image titles it.
    std::list<Unknown120Entry> mUnknown120;
};

/**
 * Allocate and construct a loop.
 *
 * This is the creator Init() installs over g_pfnNewArena. The allocation is billed to the tag
 * "Rnd::Arena" and takes 0x150 bytes.
 *
 * @param name The object name.
 * @return The new loop.
 * @ghidraAddress 0x005bbb28
 */
Arena *NewArena(const HxStr &name);

/**
 * Registered class name of Rnd::Arena, the string "Arena".
 *
 * @ghidraAddress 0x00777100
 */
extern HxStr g_arenaClassName;

/**
 * Revision word Load() has most recently consumed.
 *
 * Load() stores the revision here rather than in a local, which is what lets the helpers it calls
 * test it.
 *
 * @ghidraAddress 0x008e4a4c
 */
extern int g_nRndArenaLoadRevision;

/**
 * Factory the registered loop creator dispatches through.
 *
 * Init() fills it with NewArena(), and the thunk the class registry stores loads it rather than
 * calling the factory directly, which is what lets a platform layer substitute a subclass.
 *
 * @ghidraAddress 0x00777108
 */
extern Arena *(*g_pfnNewArena)(const HxStr &name);

} // namespace Rnd
