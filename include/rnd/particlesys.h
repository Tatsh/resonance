#pragma once

#include <list>
#include <vector>

#include "math/color.h"
#include "math/plane.h"
#include "math/vector2.h"
#include "math/vector3.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/drawable.h"
#include "rnd/manager.h"
#include "rnd/particle.h"
#include "rnd/transformable.h"

class FailSink;
namespace Rnd {
class Mat;
class Object;
class Stream;
} // namespace Rnd

namespace Rnd {

/**
 * Emitter that spawns, integrates, and draws a population of particles.
 *
 * `Q23Rnd11ParticleSys` in the RTTI descriptor at `0x008ef650`, with three public non-virtual
 * bases: `Rnd::Animatable` at `+0x00`, `Rnd::Transformable` at `+0x20`, and `Rnd::Drawable` at
 * `+0xd0`. All three derive virtually from `Rnd::Object`, so one shared Object subobject sits at
 * `+0x200` and the factory allocates 0x220 bytes.
 *
 * Four vtables belong to the class, each identified by its own GetTypeInfo slot addressing
 * `0x0052b2d8`. The Object subobject table at `0x00827b60` adjusts by `-0x200`, the Drawable table
 * at `0x00827af0` by `-0xd0`, the Transformable table at `0x00827b18` by `-0x20`, and the
 * Animatable table at `0x00827b38` by zero.
 *
 * The class overrides seven of the eight Object virtuals and two of the three Animatable ones. It
 * overrides nothing of Drawable or Transformable, so its Drawable table still addresses the base
 * DrawSelf() at `0x005066f0`, which draws nothing. Drawing belongs to Rnd::PsParticleSys.
 *
 * Particles live in two places at once. The pool is a vector of 0x80-byte records that the
 * constructor sizes to ten. The live set is a doubly linked list threaded through Particle::mNext
 * and Particle::mPrev from mLiveParticles, and the unused records of the pool form a free list
 * threaded through Particle::mNext from mFreeParticles of the owning system. AllocParticle() and
 * FreeParticle() move one record between the two lists, and each receives a system in $a0 and
 * reads mParticlesOwner, mLiveParticles, and the free list of the owner through it. Releasing a
 * particle that is already free reports "Tried to refree particle from ".
 *
 * Geometry is shared rather than copied, in the same arrangement Rnd::Mesh uses. A system whose
 * mParticlesOwner is another system draws that system's particles.
 *
 * The parameter block from `+0x108` to `+0x1f3` is named from the text dump at `0x00521f40`, which
 * reads each member under its label in the order "life:", " posLow:", " posHigh:", "speed:",
 * " pitch:", " yaw:", "emitRate:", " size:", the four colours, " collide:", " collidePlane:",
 * "force:", " mat:", " mode:", "numParticles:", " lineLength:", "bubblePeriod:", " bubbleSize:",
 * "bubble:", and " readZ:". The dump reads "numParticles:" from the size of the pool.
 */
class ParticleSys : public Animatable, public Transformable, public Drawable {
public:
    /**
     * Primitive a system draws each particle as.
     *
     * The three literals come from the mode printer at `0x0052c6a8`, which writes "Point" for
     * zero and "Line" for one, and from the unreferenced literal "Sprite" at `0x00827950`. The
     * printer has no case for the third value, so a sprite system dumps no mode at all. That gap
     * is in the shipped build rather than in this reconstruction.
     */
    enum Mode {
        kModePoint = 0,  /*!< One GS point per particle. */
        kModeLine = 1,   /*!< One GS line from mPrevPos to mPos per particle. */
        kModeSprite = 2, /*!< One GS sprite per particle. */
    };

    /** Copy() flag that shares the source's particles rather than copying its pool. */
    static constexpr unsigned kCopyShareParticles = 0x400;

    /**
     * Construct an empty system that owns its own particles.
     *
     * Sizes the pool to ten particles, points mParticlesOwner at this system, writes the default
     * parameters, and then threads the free list through AddObjectRefs().
     *
     * @param name The object name, passed to the Rnd::Object constructor.
     * @ghidraAddress 0x005254a0
     */
    ParticleSys(const HxStr &name);

    /** @ghidraAddress 0x00524f58 */
    virtual ~ParticleSys();

    /**
     * Allocate a particle system under the tag "Rnd::ParticleSys".
     *
     * @param nSize The object size the compiler supplies.
     * @return The block.
     * @ghidraAddress 0x0052b340
     */
    static void *operator new(size_t nSize);

    /**
     * Release a particle system block under the same tag.
     *
     * @param pBlock The block.
     * @ghidraAddress 0x0052b360
     */
    static void operator delete(void *pBlock);

    /**
     * Write the system to the engine text sink.
     *
     * @param sink The text sink.
     * @ghidraAddress 0x00521f40
     */
    virtual void DumpText(FailSink &sink);

    /**
     * Serialise the system.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x00522d60
     */
    virtual void Save(Stream &stream);

    /**
     * Replace one object reference with another.
     *
     * Forwards to the three bases, then retargets mMat and mParticlesOwner. Losing the particle
     * owner to a null replacement copies the owner's pool and makes this system its own owner.
     *
     * @param pFrom The object being replaced.
     * @param pTo The object to point at, which may be null.
     * @ghidraAddress 0x00524318
     */
    virtual void Replace(Object *pFrom, Object *pTo);

    /**
     * Return the registered class name, "ParticleSys".
     *
     * @return The class name.
     * @ghidraAddress 0x0052b4a8
     */
    virtual const HxStr &ClassName() const;

    /**
     * Copy another system over this one.
     *
     * Forwards to the three bases, releases this system's object references, and copies every
     * parameter except mLineLength. Without kCopyShareParticles a source that is its own particle
     * owner gives this system a copy of the pool. Otherwise this system shares the source's owner,
     * and its own pool is emptied unless that owner is this system. The references are then taken
     * again.
     *
     * @param pSource The source object. The binary dereferences the cast result without a null
     * check, so a source that is not a system faults.
     * @param nFlags The copy flags.
     * @ghidraAddress 0x00521d38
     */
    virtual void Copy(const Object *pSource, unsigned nFlags);

    /**
     * Load the system.
     *
     * Reports "Can't load new ParticleSys" through the failure sink when the file version exceeds
     * what this build writes.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x00523718
     */
    virtual void Load(Stream &stream);

    /**
     * Restart the emission and release every live particle.
     *
     * Rnd::Animatable vtable slot 2. Releases the live list and then chains to
     * Rnd::Animatable::StartAnim(). This is the only override of that slot in the shipped build,
     * so it is also the evidence the slot's title rests on.
     *
     * @ghidraAddress 0x0052c490
     */
    virtual void StartAnim();

    // The three members below are public because code outside this hierarchy calls all three
    // directly. Rnd::Generator::Regenerate() at `0x0045a998` calls every one of them, and the
    // stage classes between `0x00412000` and `0x00457000` supply nineteen further callers of
    // FreeAllParticles() and seven of AllocParticle(). A friend declaration per calling class fits
    // the image equally well, and nothing in the image distinguishes the two readings.

    /**
     * Release every live particle back to the pool of its owner.
     *
     * The walk unlinks each particle from the live list and pushes it onto the free list of
     * mParticlesOwner. A particle whose mPrev is null is already free, and releasing one reports
     * "Tried to refree particle from " with the name of the system. The head of the live list
     * stores its own address in mPrev as the marker that distinguishes it from a free particle.
     *
     * @ghidraAddress 0x00524a70
     */
    void FreeAllParticles();

    /**
     * Take one particle from the pool of the owner and push it onto the live list.
     *
     * The free list is threaded through Particle::mNext and terminates at the finish pointer of
     * the pool vector, so an exhausted pool is the head equalling that pointer. The new particle
     * becomes the head of the live list and stores its own address in mPrev.
     *
     * @return The particle, or null once the pool is exhausted.
     * @ghidraAddress 0x0052c378
     */
    Particle *AllocParticle();

    /**
     * Unlink one live particle and push it onto the free list of the owner.
     *
     * A null particle yields null. A particle whose mPrev is null is already free. Releasing it
     * reports "Tried to refree particle from " with the name of the system and yields null. The
     * routine was previously labelled as a Rnd::Generator member, and a Generator is one of its
     * callers.
     *
     * @param pParticle The particle to release, or null.
     * @return The live particle that followed it, which lets a caller release while walking.
     * @ghidraAddress 0x0052c3c0
     */
    Particle *FreeParticle(Particle *pParticle);

    /**
     * Draw a random spawn colour and size for one particle.
     *
     * Each of the five values is an independent draw from the 31-bit generator at `0x0054f770`,
     * scaled by 2 to the power of -31 and interpolated from the high bound toward the low one. The
     * four colour components come from mStartColorLow against mStartColorHigh and the size from
     * mSizeLow against mSizeHigh. Neither the position nor the velocity of the particle is
     * touched.
     *
     * @param pParticle The particle to write.
     * @ghidraAddress 0x0052c530
     */
    void RandomizeColorAndSize(Particle *pParticle);

    /**
     * Point the system at a material, moving its reference registration from the old one.
     *
     * The body is inline. The only copy in the image is an out-of-line emission with no caller,
     * and the name is inferred.
     *
     * @param pMat The material, or null.
     * @ghidraAddress 0x0052c658
     */
    void SetMat(Mat *pMat);

    /**
     * Draw the particles of another system, or of this one.
     *
     * Releases the object references, stores the owner, and takes the references again. A system
     * that no longer owns its particles then empties its own pool. The body is inline. The only
     * copy in the image is an out-of-line emission with no caller, and the name is inferred.
     *
     * @param pOwner The system whose particles to draw.
     * @ghidraAddress 0x0052c288
     */
    void SetParticlesOwner(ParticleSys *pOwner);

    /**
     * Report the head of the live list.
     *
     * The out-of-line copy has no callers. Rnd::Generator::SetFrameSelf() and
     * Rnd::Generator::DrawSelf() inline it.
     *
     * @return The first live particle, or null.
     * @ghidraAddress 0x0052b4b8
     */
    Particle *GetLiveParticles() const {
        return mLiveParticles;
    }

protected:
    /**
     * Advance the emission to a frame.
     *
     * Rnd::Animatable vtable slot 3. Returns at once while mLastFrame still stores its unset
     * sentinel. Otherwise it integrates the live particles over the elapsed frames and then spawns
     * whatever the emission rate calls for.
     *
     * @param flFrame The filtered frame to animate to.
     * @ghidraAddress 0x0052c4c0
     */
    virtual void SetFrameSelf(float flFrame);

private:
    // Integrate every live particle over a span of frames and release the ones whose death frame
    // has passed. 0x00524b70.
    void UpdateParticles(float flDeltaFrames);

    // Allocate and initialise the particles the emission rate calls for over a span of frames.
    // 0x005244b0.
    void SpawnParticles(float flDeltaFrames);

    // Drop the reference on the material and on the particle owner, and remove this system from
    // the owner's sharer list. Copy() calls it, and the destructor and Replace() open-code it.
    // 0x0052c318.
    void RemoveObjectRefs();

    // Take the references RemoveObjectRefs() drops. A system that is its own owner also threads
    // the whole pool onto its free list and empties the live list of every sharer, and any other
    // system joins the sharer list of its owner. Either way this system's live list starts empty
    // and mEmitAccumulator is cleared. The constructor, Copy(), Replace(), and Load() call it.
    // 0x005241a8.
    void AddObjectRefs();

    // Data members follow the recovered offset order.

protected:
    // The first three are protected rather than private because Rnd::PsParticleSys::DrawSelf()
    // reads all three directly, the owner and its pool for the overflow guard and the live list
    // both to test for emptiness and to walk.

    // System whose particles this one draws, itself for a system that owns them. +0xe4
    ParticleSys *mParticlesOwner;

public:
    /*!< Pool the live list draws from, which the constructor sizes to ten records of 0x80 bytes.
         Public rather than protected because Rnd::PsParticleSys::DrawSelf() measures it through
         mParticlesOwner, which is a `Rnd::ParticleSys *` and need not be a PsParticleSys, and
         protected access cannot reach a member through a pointer to the base type. The image
         exposes no accessor for it. +0xe8 */
    std::vector<Particle> mParticles;

protected:
    // Head of the live list, threaded through Particle::mNext. Rnd::PsParticleSys::DrawSelf()
    // treats a null head as nothing to draw.
    Particle *mLiveParticles;

private:
    // Head of the free list, threaded through Particle::mNext and ending at the finish pointer of
    // mParticles. Only the owning system's list is used.
    Particle *mFreeParticles;
    // Frame SetFrameSelf() last ran for. It starts at the sentinel -0.9997e7, whose bit pattern is
    // 0xcb18967f, and a frame equal to it makes SetFrameSelf() return without emitting.
    float mLastFrame;
    // Particles owed by the emission rate, of which SpawnParticles() emits the whole part and
    // retains the fraction for the next call. The title is inferred.
    float mEmitAccumulator;
    // Systems that share this one's particles. RemoveObjectRefs() removes this system from the
    // list of whichever system owns its particles.
    std::list<ParticleSys *> mSharers;
    // The range pairs below print as "(x: y:)" and store the low end in x and the high end in y.
    Vector2 mBubblePeriod;
    Vector2 mBubbleSize;
    Vector2 mLife;
    Vector3 mPosLow;
    Vector3 mPosHigh;
    Vector2 mSpeed;
    Vector2 mPitch;
    Vector2 mYaw;

public:
    /*!< Low end of the emission rate range. Public because
         Rnd::ParticleSysAnim::SetFrameSelf() writes it from outside the hierarchy and the image
         exposes no accessor. A friend declaration fits the image equally well. +0x158 */
    float mEmitRateLow;
    /*!< High end of the emission rate range. Public on the same evidence as mEmitRateLow. +0x15c */
    float mEmitRateHigh;
    /*!< Low end of the size a particle spawns with, which RandomizeColorAndSize() draws against
         mSizeHigh. Public on the same evidence as mEmitRateLow. +0x160 */
    float mSizeLow;
    /*!< High end of the size a particle spawns with. +0x164 */
    float mSizeHigh;

private:
    unsigned char mUnknown168[0x08]; // +0x168

public:
    /*!< Low end of the colour a particle spawns with. Public on the same evidence as
         mEmitRateLow. +0x170 */
    Color mStartColorLow;
    /*!< High end of the colour a particle spawns with. +0x180 */
    Color mStartColorHigh;
    /*!< Low end of the colour a particle fades to. +0x190 */
    Color mEndColorLow;
    /*!< High end of the colour a particle fades to. +0x1a0 */
    Color mEndColorHigh;

private:
    int mCollide;                   // Non-zero to collide with mCollidePlane.
    unsigned char mUnknown1b4[0xc]; // +0x1b4 Never read or written by a recovered routine.
    Plane mCollidePlane;
    Vector3 mForce;

public:
    /*!< Material every particle draws with, or null for the default surface. Public because
         Rnd::PsParticleSys::DrawSelf() selects it through a `Rnd::Mat *` and the image exposes no
         accessor. +0x1e0 */
    Mat *mMat;
    /*!< Primitive each particle draws as. Public on the same evidence as mMat, the draw path
         switching on it directly. +0x1e4 */
    Mode mMode;

private:
    int mBubble; // Non-zero to apply mBubblePeriod and mBubbleSize.

public:
    /*!< Non-zero to depth test the particles. The draw path programs TEST_1.ZTST from it, GREATER
         when set and ALWAYS when clear. +0x1ec */
    int mReadZ;
    /*!< The dump label " lineLength:" names it. The draw path passes it to PackParticleQuads(),
         whose line mode takes the second end point of each line from the quadword this many past
         Particle::mPos. +0x1f0 */
    int mLineLength;
};

/**
 * Allocate and construct a particle system.
 *
 * This is the creator the class registers with Rnd::Manager, invoked through the hook below.
 *
 * @param name The object name.
 * @return The new system.
 * @ghidraAddress 0x0052b768
 */
ParticleSys *NewParticleSys(const HxStr &name);

/**
 * Build a system for the registered "ParticleSys" class.
 *
 * Calls through g_pfnNewParticleSys and narrows the result to its Rnd::Object subobject, or to
 * null for a null result.
 *
 * @param name The object name.
 * @return The new system, as its Rnd::Object subobject.
 * @ghidraAddress 0x0052b6d8
 */
Object *CreateRegisteredParticleSys(const HxStr &name);

/**
 * Creator the registered "ParticleSys" class builds through.
 *
 * GfxDevice::Init() overwrites the hook with the Rnd::PsParticleSys creator, so a system loaded
 * from a file on the PlayStation 2 is a PsParticleSys.
 *
 * @ghidraAddress 0x0071aef8
 */
extern ParticleSys *(*g_pfnNewParticleSys)(const HxStr &name);

/**
 * Registered class name of Rnd::ParticleSys, the string "ParticleSys".
 *
 * @ghidraAddress 0x0071aef0
 */
extern HxStr g_particleSysClassName;

/**
 * Point g_pfnNewParticleSys at NewParticleSys() and register the "ParticleSys" class.
 *
 * The routine is inline. It has an out-of-line copy in this unit and another in the
 * Rnd::PsParticleSys unit at `0x005ffa38`, neither with a caller, and GfxDevice::Terminate()
 * expands the same sequence. The name is inferred.
 *
 * @ghidraAddress 0x0052b380
 */
inline void RegisterParticleSysClass() {
    g_pfnNewParticleSys = NewParticleSys;
    g_manager.RegisterClass(g_particleSysClassName, CreateRegisteredParticleSys);
}

} // namespace Rnd
