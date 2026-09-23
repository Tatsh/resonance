#pragma once

#include <list>
#include <stddef.h>

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
 * uses 0x14. The last member, mMultiMeshCursor at `+0x13c`, ends exactly on the virtual base
 * subobject, so the non-virtual part needs no padding.
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
 * SetFrameSelf() expires the instances whose age passed the path span, then spawns instances while
 * the next spawn frame has not passed the current one, culling against mBirthCam and
 * mBirthSquareDist first and giving each instance a random rotation drawn from mPathVarMax and a
 * random uniform scale drawn from mScaleGenLow and mScaleGenHigh. DrawSelf() selects one of four
 * draw paths from the table of pointers to member functions at `0x0081c448`, DrawInstanceView(),
 * DrawInstanceMesh(), DrawInstanceMultiMesh(), and DrawInstanceParticle(), preferring mView, then
 * mMesh, then mMultiMesh, then mParticleSys.
 *
 * The accessors between `0x0045dbb8` and `0x0045dca0` are inline, and each out-of-line copy has no
 * callers.
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
     * The narrowing cast is not tested, so a source that is not an emitter is read through null.
     *
     * @param pSource The source object, which has to be an emitter.
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
     * Rnd::Manager::Init() also expands this inline.
     *
     * @ghidraAddress 0x0045dcc0
     */
    static void Init();

    /**
     * Allocate an emitter under the tag "Rnd::Generator".
     *
     * @param nSize The object size the compiler supplies.
     * @return The block.
     * @ghidraAddress 0x0045db78
     */
    static void *operator new(size_t nSize);

    /**
     * Release an emitter under the tag "Rnd::Generator".
     *
     * @param pBlock The block.
     * @ghidraAddress 0x0045db98
     */
    static void operator delete(void *pBlock);

    /**
     * Count the live instances.
     *
     * The program lists no caller.
     *
     * @return The length of the instance list.
     * @ghidraAddress 0x0045e2a8
     */
    int NumInstances();

    /**
     * Make a mesh the drawn subject.
     *
     * Releases the reference on the previous mesh, takes one on the new one, and clears mView,
     * mMultiMesh, and mParticleSys, releasing each. The program lists no caller, and the title is
     * inferred.
     *
     * @param pMesh The mesh, which may be null.
     * @ghidraAddress 0x0045e698
     */
    void SetMesh(Mesh *pMesh);

    /**
     * Make a view the drawn subject, clearing mMesh, mMultiMesh, and mParticleSys.
     *
     * The program lists no caller, and the title is inferred.
     *
     * @param pView The view, which may be null.
     * @ghidraAddress 0x0045e738
     */
    void SetView(View *pView);

    /**
     * Make a multi-mesh the drawn subject, clearing mMesh, mView, and mParticleSys.
     *
     * The program lists no caller, and the title is inferred.
     *
     * @param pMultiMesh The multi-mesh, which may be null.
     * @ghidraAddress 0x0045e7d8
     */
    void SetMultiMesh(MultiMesh *pMultiMesh);

    /**
     * Make a particle system the drawn subject, clearing mMesh, mView, and mMultiMesh.
     *
     * Calls Regenerate() afterwards, so every live instance gains a particle. The program lists
     * no caller, and the title is inferred.
     *
     * @param pParticleSys The particle system, which may be null.
     * @ghidraAddress 0x0045e878
     */
    void SetParticleSys(ParticleSys *pParticleSys);

    /**
     * Set the camera the birth culling measures against.
     *
     * The program lists no caller, and the title is inferred.
     *
     * @param pCam The camera, which may be null.
     * @ghidraAddress 0x0045ea18
     */
    void SetBirthCam(Cam *pCam);

    /**
     * Replace the path and set the frame span the instances travel.
     *
     * Moves the reference from the previous path to the new one. A bound of -1 takes the matching
     * end of the path's keyframe range, from TransAnim::StartFrame() or TransAnim::EndFrame(), and
     * any other value is stored as given. AppTunnel calls it.
     *
     * @param pPath The path, or null.
     * @param flStartFrame The path frame an instance starts at, or -1.
     * @param flEndFrame The path frame an instance ends at, or -1.
     * @ghidraAddress 0x0045e920
     */
    void SetPath(TransAnim *pPath, float flStartFrame, float flEndFrame);

    /**
     * Report the path.
     *
     * @return The path, or null.
     * @ghidraAddress 0x0045dbb8
     */
    TransAnim *GetPath() const {
        return mPath;
    }

    /**
     * Report the path frame an instance starts at.
     *
     * @return The frame.
     * @ghidraAddress 0x0045dbc0
     */
    float GetPathStartFrame() const {
        return mPathStartFrame;
    }

    /**
     * Report the path frame an instance ends at.
     *
     * @return The frame.
     * @ghidraAddress 0x0045dbc8
     */
    float GetPathEndFrame() const {
        return mPathEndFrame;
    }

    /**
     * Report the mesh subject.
     *
     * @return The mesh, or null.
     * @ghidraAddress 0x0045dbd0
     */
    Mesh *GetMesh() const {
        return mMesh;
    }

    /**
     * Report the view subject.
     *
     * @return The view, or null.
     * @ghidraAddress 0x0045dbd8
     */
    View *GetView() const {
        return mView;
    }

    /**
     * Report the multi-mesh subject.
     *
     * @return The multi-mesh, or null.
     * @ghidraAddress 0x0045dbe0
     */
    MultiMesh *GetMultiMesh() const {
        return mMultiMesh;
    }

    /**
     * Report the particle system subject.
     *
     * @return The particle system, or null.
     * @ghidraAddress 0x0045dbe8
     */
    ParticleSys *GetParticleSys() const {
        return mParticleSys;
    }

    /**
     * Set mAnimateFromStart.
     *
     * @param nAnimateFromStart Non-zero to drive mView to each instance's age.
     * @ghidraAddress 0x0045dbf0
     */
    void SetAnimateFromStart(int nAnimateFromStart) {
        mAnimateFromStart = nAnimateFromStart;
    }

    /**
     * Report mAnimateFromStart.
     *
     * @return Non-zero when mView is driven to each instance's age.
     * @ghidraAddress 0x0045dbf8
     */
    int GetAnimateFromStart() const {
        return mAnimateFromStart;
    }

    /**
     * Set mBirthFrontOnly.
     *
     * @param nBirthFrontOnly Non-zero to spawn only in front of mBirthCam.
     * @ghidraAddress 0x0045dc00
     */
    void SetBirthFrontOnly(int nBirthFrontOnly) {
        mBirthFrontOnly = nBirthFrontOnly;
    }

    /**
     * Report mBirthFrontOnly.
     *
     * @return Non-zero when instances spawn only in front of mBirthCam.
     * @ghidraAddress 0x0045dc08
     */
    int GetBirthFrontOnly() const {
        return mBirthFrontOnly;
    }

    /**
     * Set mBirthSquareDistCull.
     *
     * @param nCull Non-zero to apply the mBirthSquareDist cull.
     * @ghidraAddress 0x0045dc10
     */
    void SetBirthSquareDistCull(int nCull) {
        mBirthSquareDistCull = nCull;
    }

    /**
     * Report mBirthSquareDistCull.
     *
     * @return Non-zero when the mBirthSquareDist cull applies.
     * @ghidraAddress 0x0045dc18
     */
    int GetBirthSquareDistCull() const {
        return mBirthSquareDistCull;
    }

    /**
     * Set mBirthSquareDist.
     *
     * @param flSquareDist The squared distance from mBirthCam beyond which nothing spawns.
     * @ghidraAddress 0x0045dc20
     */
    void SetBirthSquareDist(float flSquareDist) {
        mBirthSquareDist = flSquareDist;
    }

    /**
     * Report mBirthSquareDist.
     *
     * @return The squared distance from mBirthCam beyond which nothing spawns.
     * @ghidraAddress 0x0045dc28
     */
    float GetBirthSquareDist() const {
        return mBirthSquareDist;
    }

    /**
     * Report the birth camera.
     *
     * @return The camera, or null.
     * @ghidraAddress 0x0045dc30
     */
    Cam *GetBirthCam() const {
        return mBirthCam;
    }

    /**
     * Set mNextSpawnFrame.
     *
     * @param flFrame The frame the next instance spawns on.
     * @ghidraAddress 0x0045dc38
     */
    void SetNextSpawnFrame(float flFrame) {
        mNextSpawnFrame = flFrame;
    }

    /**
     * Set the bounds of the random spawn interval.
     *
     * @param flLow The low bound.
     * @param flHigh The high bound.
     * @ghidraAddress 0x0045dc40
     */
    void SetRateGen(float flLow, float flHigh) {
        mRateGenHigh = flHigh;
        mRateGenLow = flLow;
    }

    /**
     * Report the bounds of the random spawn interval.
     *
     * @param flLow Receives the low bound.
     * @param flHigh Receives the high bound.
     * @ghidraAddress 0x0045dc50
     */
    void GetRateGen(float &flLow, float &flHigh) const {
        flLow = mRateGenLow;
        flHigh = mRateGenHigh;
    }

    /**
     * Set the bounds of the random instance scale.
     *
     * @param flLow The low bound.
     * @param flHigh The high bound.
     * @ghidraAddress 0x0045dc68
     */
    void SetScaleGen(float flLow, float flHigh) {
        mScaleGenHigh = flHigh;
        mScaleGenLow = flLow;
    }

    /**
     * Report the bounds of the random instance scale.
     *
     * @param flLow Receives the low bound.
     * @param flHigh Receives the high bound.
     * @ghidraAddress 0x0045dc78
     */
    void GetScaleGen(float &flLow, float &flHigh) const {
        flLow = mScaleGenLow;
        flHigh = mScaleGenHigh;
    }

    /**
     * Set the largest random rotation about each axis, in degrees.
     *
     * @param flX The bound about x.
     * @param flY The bound about y.
     * @param flZ The bound about z.
     * @ghidraAddress 0x0045dc90
     */
    void SetPathVarMax(float flX, float flY, float flZ) {
        mPathVarMax[0] = flX;
        mPathVarMax[1] = flY;
        mPathVarMax[2] = flZ;
    }

    /**
     * Report the largest random rotation about each axis, in degrees.
     *
     * @param flX Receives the bound about x.
     * @param flY Receives the bound about y.
     * @param flZ Receives the bound about z.
     * @ghidraAddress 0x0045dca0
     */
    void GetPathVarMax(float &flX, float &flY, float &flZ) const {
        flX = mPathVarMax[0];
        flY = mPathVarMax[1];
        flZ = mPathVarMax[2];
    }

protected:
    /**
     * Draw every live instance.
     *
     * Rnd::Drawable vtable slot 3. Nothing is drawn without a path or a subject. Each instance is
     * placed on the path at its age, scaled, and composed with its mXfmMod, then drawn through the
     * selected path. A multi-mesh or particle system subject then draws once for all instances.
     *
     * @return Non-zero when the children are to be drawn as well.
     * @ghidraAddress 0x0045b040
     */
    virtual int DrawSelf();

    /**
     * Expire and spawn instances for a frame.
     *
     * Rnd::Animatable vtable slot 3. The first call only records the frame. A spawn that the birth
     * camera culls ends the call, leaving later spawns for the next frame.
     *
     * @param flFrame The filtered frame to animate to.
     * @ghidraAddress 0x0045aa40
     */
    virtual void SetFrameSelf(float flFrame);

private:
    // 0x0045e528
    // Drops the reference on every object member and empties mInstances. The
    // destructor and Copy() invoke it, and the title is inferred.
    void ReleaseRefs();

    // 0x0045e5e0
    // Takes a reference on every object member and then calls Regenerate(). Copy()
    // invokes it, and the title is inferred.
    void AcquireRefs();

    // The four draw paths DrawSelf() selects from the table at 0x0081c448. Each receives the
    // composed transform of one instance and the age of that instance in frames.

    // 0x0045ea70
    // Installs the transform as the local transform of mView, drives mView to the age
    // when mAnimateFromStart is set, recomposes, and draws.
    void DrawInstanceView(const Transform &xfm, float flAge);

    // 0x0045eb00
    // Installs the transform as the local transform of mMesh, recomposes, and draws.
    void DrawInstanceMesh(const Transform &xfm, float flAge);

    // 0x0045eb78
    // Stores the transform in the entry of the transform list of mMultiMesh that
    // mMultiMeshCursor addresses and advances the cursor.
    void DrawInstanceMultiMesh(const Transform &xfm, float flAge);

    // 0x0045ebb8
    // Moves the particle mParticleCursor addresses to the translation of the
    // transform and advances the cursor, and does nothing once the cursor is null.
    void DrawInstanceParticle(const Transform &xfm, float flAge);

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

public:
    /**
     * Frame the next instance is spawned on.
     *
     * The constructor writes the sentinel -9999999.0, and the first SetFrameSelf() replaces it with
     * the frame it receives. The dump omits it, so the title is inferred from SetFrameSelf().
     * Public because TnlBumpFX::Start() at `0x0043dea8` zeroes it, and the image has no accessor.
     * +0x108
     */
    float mNextSpawnFrame;

private:
    int mBirthFrontOnly; // +0x10c
    // +0x110 Whether SetFrameSelf() applies the mBirthSquareDist cull. The constructor zeroes it,
    // and neither the dump nor the serialiser touches it, so the title is inferred.
    int mBirthSquareDistCull;
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
    // Entry of the transform list of mMultiMesh the multi-mesh draw path writes next. DrawSelf()
    // rewinds it to the head of the list, and only the multi-mesh draw path reads it. The title is
    // inferred.
    std::list<Transform>::iterator mMultiMeshCursor; // +0x13c
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
