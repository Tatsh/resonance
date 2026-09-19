#pragma once

#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/object.h"
#include "rnd/particlesys.h"
#include "rnd/stream.h"

namespace Rnd {

/**
 * Keyframed animation of one Rnd::ParticleSys.
 *
 * `Q23Rnd15ParticleSysAnim` in the RTTI descriptor at `0x008f0eb0`, with `Rnd::Animatable` as its
 * one public base at offset 0. The shared Object subobject sits at `+0x30`, which the `-0x30`
 * adjustment on every entry of the Object subobject table confirms, so the members below occupy
 * `+0x18` through `+0x2f` and the Animatable subobject is the 0x18 bytes ahead of them. The creator
 * allocates exactly 0x4c bytes, which is that layout with nothing left over.
 *
 * Two vtables belong to the class, each identified by its own GetTypeInfo slot addressing
 * `0x0052b7f0`. The Object subobject table at `0x00827a80` stores the seven Object overrides, and
 * the Animatable table at `0x00827ac8` stores EndFrame() at slot 1 and SetFrameSelf() at slot 3.
 * Slot 2 still addresses the base StartAnim() at `0x0049a3b8`, so restarting an animation does
 * nothing of its own here.
 *
 * Three channels drive the system, and the text dump titles them "startColorKeys:",
 * "endColorKeys:", and "emitRateKeys:". Each is one word holding the sentinel of a `std::list`,
 * which the two channel dumps prove by reading the word and then comparing the first node against
 * it. The two colour channels share a dump routine at `0x004d8de8` and the emission rate channel
 * has its own at `0x004d8f08`, so the element types differ.
 *
 * Keys are shared rather than copied, the same arrangement Rnd::MeshAnim uses with mKeysOwner.
 * Both EndFrame() and SetFrameSelf() read the channels of mFramesOwner rather than their own, so
 * an animation whose frames belong to another animation reads that object's keys.
 *
 * Recovery is partial. The keyframe record of each channel is not reconstructed, and the only
 * offset inside it that is pinned is the frame at `+0x18` of the element, which both overrides read
 * through the list node at `+0x20`. Every routine that walks a channel therefore stays
 * unreconstructed: EndFrame() at `0x00527128`, SetFrameSelf() at `0x00527290`, Save() at
 * `0x00526b68`, Load() at `0x00526ce8`, Replace() at `0x005267a8`, Copy() at `0x00526fc8`, the
 * constructor at `0x0052bca0`, and the destructor at `0x0052b9c0`. The same gap keeps Rnd::MeshAnim
 * and Rnd::MatAnim from reconstructing their channels.
 */
class ParticleSysAnim : public Animatable {
public:
    /**
     * Construct an animation with no channels and no system.
     *
     * The body is not reconstructed. Rnd::NewParticleSysAnim() is the only construction site.
     *
     * @param name The object name, passed to the Rnd::Object constructor.
     * @ghidraAddress 0x0052bca0
     */
    ParticleSysAnim(const HxStr &name);

    /** @ghidraAddress 0x0052b9c0 */
    virtual ~ParticleSysAnim();

    /**
     * Write the animation to the engine text sink.
     *
     * Emits the Object and Animatable dumps, then the "[ParticleSysAnim]" block. The block is
     * suppressed while the dump level of the sink is not positive.
     *
     * @param sink The text sink.
     * @ghidraAddress 0x00526980
     */
    virtual void DumpText(FailSink &sink);

    /**
     * Serialise the animation.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x00526b68
     */
    virtual void Save(Stream &stream);

    /**
     * Replace one object reference with another.
     *
     * @param pFrom The object being replaced.
     * @param pTo The object to point at, which may be null.
     * @ghidraAddress 0x005267a8
     */
    virtual void Replace(Object *pFrom, Object *pTo);

    /**
     * Return the registered class name, "ParticleSysAnim".
     *
     * @return The class name.
     * @ghidraAddress 0x0052bc90
     */
    virtual const HxStr &ClassName() const;

    /**
     * Copy another animation over this one.
     *
     * @param pSource The source object, which has to be an animation for the copy to have any
     *                effect.
     * @param nFlags The copy flags.
     * @ghidraAddress 0x00526fc8
     */
    virtual void Copy(const Object *pSource, unsigned nFlags);

    /**
     * Load the animation.
     *
     * Reports "Can't load new ParticleSysAnim" through the failure sink when the file version
     * exceeds what this build writes.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x00526ce8
     */
    virtual void Load(Stream &stream);

    /**
     * Report the last frame this animation runs to.
     *
     * Rnd::Animatable vtable slot 1. Counts the keys of each channel of mFramesOwner and reports
     * the frame of the last key across the three, taking the frame from the final list node of
     * each. An empty channel contributes zero.
     *
     * @return The last frame.
     * @ghidraAddress 0x00527128
     */
    virtual float EndFrame();

protected:
    /**
     * Animate the system to a frame.
     *
     * Rnd::Animatable vtable slot 3. Returns at once when mParticleSys is null. Otherwise it
     * interpolates each channel of mFramesOwner at the frame and writes the results into the
     * system.
     *
     * @param flFrame The filtered frame to animate to.
     * @ghidraAddress 0x00527290
     */
    virtual void SetFrameSelf(float flFrame);

private:
    // No class derives from Rnd::ParticleSysAnim and no access from outside it is recovered, so
    // every member is private. The order below is the recovered offset order, and each member is
    // pinned by the text dump, which reads all six.

    // System this animation drives. SetFrameSelf() returns without doing anything when it is null.
    ParticleSys *mParticleSys; // +0x18
    // Sentinel of the channel that drives the spawn colour at the low end of its range.
    int mStartColorKeys; // +0x1c
    // Sentinel of the channel that drives the spawn colour at the high end of its range.
    int mEndColorKeys; // +0x20
    // Sentinel of the channel that drives the emission rate.
    int mEmitRateKeys; // +0x24
    // Animation whose channels this one reads, itself for an animation that owns its keys.
    ParticleSysAnim *mFramesOwner; // +0x28
    // Scale the emission rate channel is multiplied by.
    float mEmitRateRatio; // +0x2c
};

/**
 * Allocate and construct a particle system animation.
 *
 * This is the creator the class registers with Rnd::Manager. Unlike Rnd::ParticleSys, the class
 * installs no creator hook, so there is one creator and no platform subclass.
 *
 * @param name The object name.
 * @return The new animation.
 * @ghidraAddress 0x0052c180
 */
ParticleSysAnim *NewParticleSysAnim(const HxStr &name);

/**
 * Registered class name of Rnd::ParticleSysAnim, the string "ParticleSysAnim".
 *
 * @ghidraAddress 0x0071af00
 */
extern HxStr g_particleSysAnimClassName;

} // namespace Rnd
