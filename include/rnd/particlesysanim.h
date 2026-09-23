#pragma once

#include <list>

#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/keychannel.h"
#include "rnd/manager.h"

class FailSink;
namespace Rnd {
class Object;
class ParticleSys;
class Stream;
} // namespace Rnd

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
 * "endColorKeys:", and "emitRateKeys:". The two colour channels are `std::list` of Rnd::ColorKey
 * and share the dump routine at `0x004d8de8`. The emission rate channel is a `std::list` of
 * Rnd::FloatKey and has its own dump at `0x004d8f08`. EndFrame() reads the frame of the two colour
 * channels at list node `+0x20` and of the rate channel at node `+0x0c`, which is the two element
 * sizes rather than one.
 *
 * Keys are shared rather than copied, the same arrangement Rnd::MeshAnim uses with mKeysOwner.
 * Both EndFrame() and SetFrameSelf() read the channels of mFramesOwner rather than their own, so
 * an animation whose frames belong to another animation reads that object's keys.
 *
 * SetFrameSelf() is blocked rather than unrecovered; see its own documentation.
 */
class ParticleSysAnim : public Animatable {
public:
    /** Revision Save() writes, and the highest revision Load() accepts. */
    enum { kSerialVersion = 1 };

    /**
     * Bit of the copy flags that shares the source's keyframe channels rather than copying them.
     *
     * Recovered from the `andi` at `0x0052706c` in Copy(). Rnd::MeshAnim reads bit 0x40 and
     * Rnd::LightAnim bit 0x02 for the same purpose, so the bit is per class rather than shared
     * across the hierarchy.
     */
    enum { kCopyShareKeys = 0x80 };

    /**
     * Construct an animation with no channels and no system that owns its own frames.
     *
     * mEmitRateRatio starts at zero. Rnd::NewParticleSysAnim() is the only construction site.
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
     * Writes kSerialVersion, the Rnd::Animatable subobject, mParticleSys as a name, the three
     * channels, mFramesOwner as a name, and finally mEmitRateRatio.
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
     * kCopyShareKeys shares the source's channels instead of copying them, and a source that is
     * itself sharing is always shared from rather than copied. mEmitRateRatio is copied either
     * way.
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
     * Rnd::Animatable vtable slot 3. Returns at once when mParticleSys is null. Each of the two
     * colour channels of mFramesOwner selects the bracketing keyframe pair, blends the pair on
     * VU0, and then moves one spawn colour range of the system so that the blended colour becomes
     * the low end while the spread of the range survives. The move is AddColor() followed by
     * SubColor().
     *
     * The emission rate channel interpolates its two scalars linearly instead, then divides
     * Rnd::ParticleSys::mEmitRateHigh by mEmitRateLow into mEmitRateRatio whenever the second is
     * not zero, and finally writes the interpolated rate to mEmitRateLow and that rate scaled by
     * mEmitRateRatio to mEmitRateHigh. An empty rate channel finishes the routine.
     *
     * @param flFrame The filtered frame to animate to.
     * @ghidraAddress 0x00527290
     */
    virtual void SetFrameSelf(float flFrame);

    /**
     * Make a particle system the one this animation drives.
     *
     * Moves this object's reference from the previous system to the new one. No call site
     * survives in the shipped program, and the name is inferred.
     *
     * @param pParticleSys The new system, or null.
     * @ghidraAddress 0x0052c700
     */
    void SetParticleSys(ParticleSys *pParticleSys);

    /**
     * Make another animation the one whose keyframes drive this one.
     *
     * Moves this object's reference from the previous owner to the new one, and then empties this
     * object's own channels unless it is its own owner. No call site survives in the shipped
     * program, and the name follows the Rnd::TransAnim counterpart.
     *
     * @param pOwner The new frames owner, or null.
     * @ghidraAddress 0x0052c7a0
     */
    void SetFramesOwner(ParticleSysAnim *pOwner);

private:
    // Empty the three channels unless this animation owns its frames. Load() and SetFramesOwner()
    // inline it. 0x0052c758.
    void ClearKeys();

    // Register this animation as a referrer of mParticleSys and mFramesOwner. Load() inlines it.
    // 0x0052c868.
    void AddObjectRefs();

    // Drop the references AddObjectRefs() took. The destructor calls it. 0x0052c818.
    void RemoveObjectRefs();

    // No class derives from Rnd::ParticleSysAnim and no access from outside it is recovered, so
    // every member is private. The order below is the recovered offset order, and each member is
    // pinned by the text dump, which reads all six.

    // System this animation drives. SetFrameSelf() returns without doing anything when it is null.
    ParticleSys *mParticleSys; // +0x18
    // Channel that drives the spawn colour at the low end of its range.
    std::list<ColorKey> mStartColorKeys; // +0x1c
    // Channel that drives the spawn colour at the high end of its range.
    std::list<ColorKey> mEndColorKeys; // +0x20
    // Channel that drives the emission rate.
    std::list<FloatKey> mEmitRateKeys; // +0x24
    // Animation whose channels this one reads, itself for an animation that owns its keys.
    ParticleSysAnim *mFramesOwner; // +0x28
    // Scale the emission rate channel is multiplied by.
    float mEmitRateRatio; // +0x2c
};

/**
 * Allocate and construct a particle system animation.
 *
 * The allocation is untagged, unlike every other renderer class, and exactly 0x4c bytes. The
 * out-of-line copy has no caller, and CreateRegisteredParticleSysAnim() expands the body. An
 * exception from the constructor produces null, which is what the binary's handler returns.
 *
 * @param name The object name.
 * @return The new animation, or null.
 * @ghidraAddress 0x0052b8a8
 */
inline ParticleSysAnim *NewParticleSysAnim(const HxStr &name) {
    try {
        return new ParticleSysAnim(name);
    } catch (...) {
        return nullptr;
    }
}

/**
 * Build a particle system animation for the registered "ParticleSysAnim" class.
 *
 * Converts the result of NewParticleSysAnim() to its Rnd::Object virtual base. Unlike
 * Rnd::ParticleSys, the class installs no creator hook, so there is one creator and no platform
 * subclass.
 *
 * @param name The object name.
 * @return The new animation, as its Rnd::Object subobject.
 * @ghidraAddress 0x0052c180
 */
Object *CreateRegisteredParticleSysAnim(const HxStr &name);

/**
 * Registered class name of Rnd::ParticleSysAnim, the string "ParticleSysAnim".
 *
 * @ghidraAddress 0x0071af00
 */
extern HxStr g_particleSysAnimClassName;

/**
 * Register the "ParticleSysAnim" class with Rnd::Manager.
 *
 * The class has no creator hook to reset, so the body is the registration alone. The out-of-line
 * copy has no caller. The name is inferred.
 *
 * Rnd::Manager::Init() also expands this inline.
 *
 * @ghidraAddress 0x0052b878
 */
inline void RegisterParticleSysAnimClass() {
    g_manager.RegisterClass(g_particleSysAnimClassName, CreateRegisteredParticleSysAnim);
}

} // namespace Rnd
