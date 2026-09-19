#pragma once

#include <list>
#include <vector>

#include "math/color.h"
#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/drawable.h"
#include "rnd/mat.h"
#include "rnd/object.h"
#include "rnd/particle.h"
#include "rnd/stream.h"
#include "rnd/transformable.h"

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
 * constructor sizes to ten, and the live set is a singly linked list threaded through
 * Particle::mNext from mLiveParticles. Allocation and release go through a pool shared with
 * Rnd::Generator, whose helpers are at `0x0052c378` and `0x0052c3c0` and are therefore not members
 * of this class. Releasing a particle that the pool does not own reports "Tried to refree particle
 * from ".
 *
 * Geometry is shared rather than copied, in the same arrangement Rnd::Mesh uses. A system whose
 * mParticlesOwner is another system draws that system's particles.
 *
 * Recovery is partial. The parameter block between `+0x108` and `+0x1df` is where the spawn ranges
 * live, and only the members below are pinned, each by a routine that reads it. The remaining
 * labels the text dump writes, "life:", " posLow:", " posHigh:", "speed:", " pitch:", " yaw:",
 * "emitRate:", " size:", "startColorLow:", "startColorHigh:", "endColorLow:", "endColorHigh:",
 * " collide:", " collidePlane:", "force:", " lineLength:", "bubblePeriod:", " bubbleSize:",
 * "bubble:", and " readZ:", identify that block without pinning an offset to each. The
 * serialisation trio is not reconstructed either; DumpText() alone is 0xe20 bytes.
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

    /**
     * Construct an empty system that owns its own particles.
     *
     * Sizes the pool to ten particles, points mParticlesOwner at this system, and writes the
     * default spawn ranges.
     *
     * @param name The object name, passed to the Rnd::Object constructor.
     * @ghidraAddress 0x005254a0
     */
    ParticleSys(const HxStr &name);

    /** @ghidraAddress 0x00524f58 */
    virtual ~ParticleSys();

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
     * Forwards to the three bases, releases this system's object references, and then copies the
     * parameter block from `+0x118` through `+0x1f0` as quadwords.
     *
     * @param pSource The source object, which has to be a system for the copy to have any effect.
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

    // Release every live particle. StartAnim() and the destructor are its callers. 0x00524a70.
    void FreeAllParticles();

    // Drop the reference on the material and on the particle owner, and remove this system from
    // the owner's sharer list. Copy() is its only caller. 0x0052c318.
    void RemoveObjectRefs();

    // Data members follow the recovered offset order. Only the members below are pinned, each by a
    // routine that reads it, and the gaps record what is not.

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
    Particle *mLiveParticles; // +0xf4

private:
    int mUnknownf8; // +0xf8
    // Frame SetFrameSelf() last ran for. It starts at the sentinel -0.9997e7, whose bit pattern is
    // 0xcb18967f, and a frame equal to it makes SetFrameSelf() return without emitting.
    float mLastFrame; // +0xfc
    int mUnknown100;  // +0x100
    // Systems that share this one's particles. RemoveObjectRefs() removes this system from the
    // list of whichever system owns its particles.
    std::list<ParticleSys *> mSharers; // +0x104
    // The spawn parameter block. See the class note. Six of its fields are recovered, because
    // Rnd::ParticleSysAnim::SetFrameSelf() writes them. The dump of this block writes every other
    // range it has as a "…Low:" and "…High:" pair, "posLow:" against "posHigh:" and
    // "startColorLow:" against "startColorHigh:", and the animation shifts the high member of each
    // range by however far it moved the low member, which preserves the spread. The two emission
    // rates take their titles from that pattern rather than from a label of their own.
    unsigned char mUnknown108[0x50]; // +0x108

public:
    /*!< Low end of the emission rate range. Public because
         Rnd::ParticleSysAnim::SetFrameSelf() writes it from outside the hierarchy and the image
         exposes no accessor. A friend declaration fits the image equally well. +0x158 */
    float mEmitRateLow;
    /*!< High end of the emission rate range. Public on the same evidence as mEmitRateLow. +0x15c */
    float mEmitRateHigh;

private:
    unsigned char mUnknown160[0x10]; // +0x160

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
    unsigned char mUnknown1b0[0x30]; // +0x1b0

public:
    /*!< Material every particle draws with, or null for the default surface. Public because
         Rnd::PsParticleSys::DrawSelf() selects it through a `Rnd::Mat *` and the image exposes no
         accessor. +0x1e0 */
    Mat *mMat;
    /*!< Primitive each particle draws as. Public on the same evidence as mMat, the draw path
         switching on it directly. +0x1e4 */
    Mode mMode;

private:
    int mUnknown1e8; // +0x1e8

public:
    /*!< Non-zero to depth test the particles. The draw path programs TEST_1.ZTST from it, GREATER
         when set and ALWAYS when clear. +0x1ec */
    int mReadZ;
    /*!< Ceiling on the live population, which the text dump writes as "numParticles:". The draw
         path passes it to PackParticleQuads(). +0x1f0 */
    int mMaxParticles;
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

} // namespace Rnd
