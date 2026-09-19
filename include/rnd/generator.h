#pragma once

#include <list>

#include "math/transform.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/drawable.h"
#include "rnd/transformable.h"

namespace Rnd {
class Cam;
}
class FailSink;
namespace Rnd {
class Mesh;
}
namespace Rnd {
class MultiMesh;
}
namespace Rnd {
class Object;
}
namespace Rnd {
class ParticleSys;
}
namespace Rnd {
class Stream;
}
namespace Rnd {
class TransAnim;
}
namespace Rnd {
class View;
}
namespace Rnd {
struct Particle;
}

namespace Rnd {

/** Number of axes the random path variation covers. */
constexpr int kPathVarAxisCount = 3;

/**
 * Emitter that spawns copies of one drawable along an animated path.
 *
 * `Q23Rnd9Generator` in the RTTI descriptor at `0x008ef5e0`, whose name string is at `0x0081c640`
 * and whose three base entries record `Rnd::Animatable` at `+0x00`, `Rnd::Transformable` at
 * `+0x20`, and `Rnd::Drawable` at `+0xd0`, each non-virtual and public. All three derive virtually
 * from `Rnd::Object`, so one shared Object subobject sits at `+0x140`, which the constructor
 * proves by writing `this + 0x140` into the virtual-base pointer of each of the three subobjects.
 * The class is therefore 0x15c bytes, and the creator allocates 0x160, which is that size rounded
 * up to the quadword the vector unit reads a transform from. The members of the class itself
 * occupy `+0xe4` through `+0x13f`.
 *
 * The base offsets also pin the three base subobjects. Animatable uses 0x18 bytes, and the eight
 * bytes after it are the padding that puts the 16-byte-aligned Transformable subobject on `+0x20`.
 * Transformable uses 0xac and its size rounds to 0xb0 on its own 16-byte alignment, and Drawable
 * uses 0x14. The four bytes past the last member are the same rounding applied to the non-virtual
 * part of this class before the virtual base subobject is placed, which `Rnd::Arena` confirms
 * independently.
 *
 * Four vtables belong to the class, each identified by a GetTypeInfo slot addressing `0x0045db10`
 * and by an adjustment matching its subobject offset. The Drawable table at `0x0081c508` adjusts
 * by `-0xd0` and overrides only DrawSelf(). The Transformable table at `0x0081c530` adjusts by
 * `-0x20` and overrides nothing. The Animatable table at `0x0081c550` has a zero adjustment and
 * overrides only SetFrameSelf(). The Object subobject table at `0x0081c578` adjusts by `-0x140`
 * and stores the seven Object virtuals. Every table ends in an all-zero entry, which is the
 * terminator rather than a null slot. No table gains a slot past the own count of its base. This
 * class therefore declares no virtual of its own.
 *
 * Every member title below comes from the text DumpText() writes, so each one is recovered rather
 * than inferred: "path:", " mesh:", " birthFrontOnly:", "birthSquareDist:", " birthCam:",
 * "rateGenLow:", " rateGenHigh:", "scaleGenLow:", " scaleGenHigh:", "pathVarMax:(", "view:",
 * " animateFromStart:", "multiMesh:", " particleSys:", "instances:", "pathEndFrame:", and
 * " pathStartFrame:". The two members the dump omits but SetFrameSelf() uses are titled from what
 * SetFrameSelf() does with them, and each one records that inference. The two the dump omits and
 * no routine uses retain their offsets as titles.
 *
 * The concrete type of each of the six object references comes from the narrowing cast Load()
 * performs through the `dynamic_cast` helper at `0x005570e0`, whose target type function
 * identifies the class in every case.
 *
 * SetFrameSelf() and DrawSelf() are not reconstructed. SetFrameSelf() at `0x0045aa40` expires the
 * instances whose age passed the path span, then spawns instances while the next spawn frame has
 * not passed the current one, culling against mBirthCam and mBirthSquareDist first and giving each
 * instance a random rotation drawn from mPathVarMax and a random uniform scale drawn from
 * mScaleGenLow and mScaleGenHigh. DrawSelf() at `0x0045b040` selects one of four draw paths from
 * `0x0081c448` according to which of mMesh, mMultiMesh, and mParticleSys are set.
 */
class Generator : public Animatable, public Transformable, public Drawable {
public:
    /**
     * One live instance.
     *
     * The record is 0x60 bytes, which the 0x70-byte list node the allocation takes from the
     * eight-byte bucket at index 13 establishes together with the `+0x10` payload offset a
     * 16-byte-aligned element forces. mFrameOrg and mXfmMod are the two members the instance dump
     * at `0x0045b3d0` writes, under the labels "(frameOrg: " and " xfmMod:".
     */
    struct Instance {
        /** Frame this instance was spawned on. +0x00 */
        float mFrameOrg;
        unsigned char mUnknown04[0x0c]; // +0x04 Unrecovered. The instance dump reads none of it.
        /** Rotation and translation the instance is drawn with. +0x10 */
        Transform mXfmMod;
        // +0x50 The uniform scale SetFrameSelf() draws for this instance, written into all three
        // components. The instance dump reads none of it, so the title is inferred.
        Vector3 mScale;
    };

    /**
     * Construct an emitter with no instances and no subject.
     *
     * mAnimateFromStart starts at 1, the next spawn frame at the sentinel -9999999.0, both
     * rateGen bounds at 100.0, both scaleGen bounds at 1.0, and every other member at zero.
     *
     * @param name The registry key for this object.
     * @ghidraAddress 0x00458748
     */
    explicit Generator(const HxStr &name);

    /** @ghidraAddress 0x0045de20 */
    virtual ~Generator();

    /**
     * Write the emitter to the engine text sink.
     *
     * The three base dumps run unconditionally and the block below them only while the dump level
     * of the sink is positive. The instance list and the two path frames need level two.
     *
     * @param sink The text sink.
     * @ghidraAddress 0x00459618
     */
    virtual void DumpText(FailSink &sink);

    /**
     * Write revision 7 of the emitter to stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x00459bd8
     */
    virtual void Save(Stream &stream);

    /**
     * Replace one object reference with another.
     *
     * @param pFrom The object being replaced.
     * @param pTo The object to point at, which may be null.
     * @ghidraAddress 0x00459220
     */
    virtual void Replace(Object *pFrom, Object *pTo);

    /**
     * Return the registered class name, "Generator".
     *
     * @return The class name.
     * @ghidraAddress 0x0045e2e8
     */
    virtual const HxStr &ClassName() const;

    /**
     * Copy another emitter over this one.
     *
     * @param pSource The source object, which has to be an emitter for the copy to take effect.
     * @param nFlags The copy flags.
     * @ghidraAddress 0x0045e3d8
     */
    virtual void Copy(const Object *pSource, unsigned nFlags);

    /**
     * Read the emitter from stream.
     *
     * Rejects revision 8 and above, reporting "Can't load new Generator". Revision 7 is the only
     * one this build writes, and every earlier revision is still read.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x0045a090
     */
    virtual void Load(Stream &stream);

    /**
     * Give every live instance a fresh particle from mParticleSys.
     *
     * Does nothing while mParticleSys is null. Otherwise it releases every live particle, then
     * allocates one particle per live instance and randomises the colour and the size of each one.
     * Only the instance count is read. The loop never visits an instance, and an allocation that
     * fails skips the randomisation and continues rather than ending the loop, which corrects an
     * earlier reading of `0x0045aa0c`. mParticleCursor receives the allocation result whether or
     * not it succeeded, because that store sits in the delay slot of the test.
     *
     * @ghidraAddress 0x0045a998
     */
    void Regenerate();

    /**
     * Report the live instance list.
     *
     * The routine is two instructions that return the address of the member. It is compiled out of
     * line here, so it exists as a function rather than as an accessor this reconstruction added.
     *
     * @return The live instances.
     * @ghidraAddress 0x0045e2f8
     */
    std::list<Instance> &Instances();

    /**
     * Register the class with Rnd::g_manager under the key "Generator".
     *
     * The class installs no creator hook, unlike Rnd::Blur and Rnd::Tex, so the thunk the registry
     * stores calls the constructor directly.
     *
     * @ghidraAddress 0x0045dcc0
     */
    static void Init();

protected:
    /**
     * Draw every live instance.
     *
     * Rnd::Drawable vtable slot 3. The body is not reconstructed.
     *
     * @return Non-zero when the children are to be drawn as well.
     * @ghidraAddress 0x0045b040
     */
    virtual int DrawSelf();

    /**
     * Expire and spawn instances for a frame.
     *
     * Rnd::Animatable vtable slot 3. The body is not reconstructed.
     *
     * @param flFrame The filtered frame to animate to.
     * @ghidraAddress 0x0045aa40
     */
    virtual void SetFrameSelf(float flFrame);

private:
    // No class derives from Rnd::Generator and nothing outside it accesses a member directly.
    // Every member is therefore private. The order below is the recovered offset order.

    std::list<Instance> mInstances; // +0xe4
    // Path the spawned instances follow. Load() narrows the resolved object to Rnd::TransAnim.
    TransAnim *mPath;          // +0xe8
    float mPathStartFrame;     // +0xec
    float mPathEndFrame;       // +0xf0
    Mesh *mMesh;               // +0xf4
    View *mView;               // +0xf8
    MultiMesh *mMultiMesh;     // +0xfc
    ParticleSys *mParticleSys; // +0x100
    int mAnimateFromStart;     // +0x104
    // Frame the next instance is spawned on. The constructor writes the sentinel -9999999.0, and
    // the first SetFrameSelf() replaces it with the frame it receives. The dump omits it, so the
    // title is inferred from SetFrameSelf().
    float mNextSpawnFrame; // +0x108
    int mBirthFrontOnly;   // +0x10c
    // +0x110 Unrecovered. The constructor zeroes it, neither the dump nor the serialiser touches
    // it, and SetFrameSelf() only tests it for zero before applying the mBirthSquareDist cull.
    int mUnknown110;
    float mBirthSquareDist;               // +0x114
    Cam *mBirthCam;                       // +0x118
    float mRateGenLow;                    // +0x11c
    float mRateGenHigh;                   // +0x120
    float mScaleGenLow;                   // +0x124
    float mScaleGenHigh;                  // +0x128
    float mPathVarMax[kPathVarAxisCount]; // +0x12c
    // Particle of mParticleSys the walk currently stands on. Regenerate() stores the particle it
    // allocated and SetFrameSelf() advances it through Rnd::Particle::mNext. The dump omits it, so
    // the title is inferred from those two routines.
    Particle *mParticleCursor; // +0x138
};

/**
 * Allocate and construct an emitter.
 *
 * The allocation is billed to the tag "Rnd::Generator" and takes 0x160 bytes. Nothing in the image
 * references this routine, which is the unused out-of-line copy the toolchain emits for an inline
 * function. The thunk the registry stores is the separate routine at `0x0045e300`.
 *
 * @param name The object name.
 * @return The new emitter.
 * @ghidraAddress 0x0045dcf0
 */
Generator *NewGenerator(const HxStr &name);

/**
 * Registered class name of Rnd::Generator, the string "Generator".
 *
 * @ghidraAddress 0x006e8280
 */
extern HxStr g_generatorClassName;

} // namespace Rnd
