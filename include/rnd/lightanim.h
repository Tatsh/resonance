#pragma once

#include <list>

#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/keychannel.h"

class FailSink;
namespace Rnd {
class Light;
class Object;
class Stream;
} // namespace Rnd

namespace Rnd {

/**
 * Keyframed animation of the three colours of one Rnd::Light.
 *
 * `Q23Rnd9LightAnim` in the RTTI descriptor at `0x008ef1f0`, with `Rnd::Animatable` as its one
 * public base at offset 0. The shared `Rnd::Object` subobject sits at `+0x2c`, an offset the
 * `-0x2c` adjustment on every entry of the Object subobject table confirms. The members below
 * therefore occupy `+0x18` through `+0x2b`, and the Animatable subobject is the 0x18 bytes ahead
 * of them. The creator at `0x005452d0` allocates exactly 0x48 bytes, the sum of that 0x2c and the
 * 0x1c of the Object subobject with no surplus.
 *
 * Two vtables belong to the class, each with the accessor at `0x00544940` in slot 0. The Object
 * subobject table at `0x00829448` stores the seven Object overrides and the destructor, and the
 * Animatable table at `0x00829490` stores EndFrame() at slot 1 and SetFrameSelf() at slot 3. Slot
 * 2 still addresses the base StartAnim() at `0x0049a3b8`. Restarting an animation therefore does
 * nothing of its own here.
 *
 * Three channels drive the light, and the text dump titles them "ambientKeys:", "diffuseKeys:",
 * and "specularKeys:". SetFrameSelf() hands the three interpolated results to
 * Rnd::Light::SetColors() in that order, which is independent confirmation of that parameter
 * order. Each channel is a `std::list` of Rnd::ColorKey, the same arrangement Rnd::MatAnim and
 * Rnd::ParticleSysAnim use, and the shared channel dump at `0x004d8de8` serves all three classes.
 *
 * Keys are shared rather than copied, the same arrangement Rnd::MeshAnim uses with mKeysOwner.
 * Both EndFrame() and SetFrameSelf() read the channels of mKeysOwner rather than their own. An
 * animation whose keys belong to another animation therefore reads that object's keys.
 */
class LightAnim : public Animatable {
public:
    /** Revision Save() writes, and the highest revision Load() accepts. */
    enum { kSerialVersion = 0 };

    /**
     * Bit of the copy flags that shares the source's keyframe channels rather than copying them.
     *
     * Recovered from the `andi` at `0x005416dc` in Copy(). Rnd::MeshAnim reads bit 0x40 and
     * Rnd::ParticleSysAnim bit 0x80 for the same purpose, so the bit is per class rather than
     * shared across the hierarchy.
     */
    enum { kCopyShareKeys = 0x02 };

    /**
     * Construct an animation with three empty channels and no light, owning its keys.
     *
     * NewLightAnim() is the one construction site and allocates 0x48 untagged bytes before
     * invoking it.
     *
     * @param name The object name, passed to the Rnd::Object constructor.
     * @ghidraAddress 0x00544df0
     */
    explicit LightAnim(const HxStr &name);

    /** @ghidraAddress 0x00544b10 */
    virtual ~LightAnim();

    /**
     * Report the light this animation drives.
     *
     * The body is one load in a branch delay slot, and it is an accessor rather than an asserted
     * member only because the image emits it out of line at its own address.
     *
     * @return The light, or null while none is set.
     * @ghidraAddress 0x00544dd0
     */
    Light *GetLight();

    /**
     * Report the animation whose channels this one reads.
     *
     * @return The keys owner, which is this object for an animation that owns its keys.
     * @ghidraAddress 0x00544dd8
     */
    LightAnim *GetKeysOwner();

    /**
     * Make another animation the one whose channels drive this one.
     *
     * Moves this object's reference from the previous owner to the new one, and then empties this
     * object's own channels unless it is its own owner. No call site survives in the shipped
     * program, and the name follows GetKeysOwner().
     *
     * @param pOwner The new keys owner, or null.
     * @ghidraAddress 0x005455b8
     */
    void SetKeysOwner(LightAnim *pOwner);

    /**
     * Write the animation to the engine text sink.
     *
     * Emits the Object and Animatable dumps, then the "[LightAnim]" block. The block is suppressed
     * while the dump level of the sink is not positive.
     *
     * @param sink The text sink.
     * @ghidraAddress 0x00541078
     */
    virtual void DumpText(FailSink &sink);

    /**
     * Serialise the animation.
     *
     * Writes kSerialVersion, the Rnd::Animatable subobject, mLight as a name, the three channels,
     * and finally mKeysOwner as a name.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x00541230
     */
    virtual void Save(Stream &stream);

    /**
     * Retarget one object reference at another.
     *
     * A null pTo means the animation that owned the shared keys is going away, and the three
     * channels are taken over from that owner rather than emptied.
     *
     * @param pFrom The object being replaced.
     * @param pTo The replacement, or null.
     * @ghidraAddress 0x00540ea0
     */
    virtual void Replace(Object *pFrom, Object *pTo);

    /**
     * Report the registered class name, "LightAnim".
     *
     * @return The class name.
     * @ghidraAddress 0x00544de0
     */
    virtual const HxStr &ClassName() const;

    /**
     * Copy another animation over this one.
     *
     * kCopyShareKeys shares the source's channels instead of copying them, and a source that is
     * itself sharing is always shared from rather than copied.
     *
     * @param pSource The object to copy from.
     * @param nFlags The set of fields to copy.
     * @ghidraAddress 0x00541638
     */
    virtual void Copy(const Object *pSource, unsigned nFlags);

    /**
     * Load the animation.
     *
     * A revision above 0 produces the report "Can't load new LightAnim" and no further reading.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x00541388
     */
    virtual void Load(Stream &stream);

    /**
     * Report the last frame this animation runs to.
     *
     * Rnd::Animatable vtable slot 1. Each channel of mKeysOwner contributes the frame of its last
     * key, or zero while it is empty, and the result is the largest of the three through two
     * nested `std::max` calls. The pair of selected stack addresses rather than selected values
     * proves the two calls.
     *
     * @return The last frame.
     * @ghidraAddress 0x00541790
     */
    virtual float EndFrame();

protected:
    /**
     * Animate the light to a frame.
     *
     * Rnd::Animatable vtable slot 3. Returns at once while mLight is null. Otherwise it seeds
     * three local colours from the current ambient, diffuse, and specular of the light, then
     * replaces each from its channel of mKeysOwner. A frame at or before the first key takes that
     * key, a frame at or after the last key takes that key, and a frame between two keys takes the
     * linear blend of the pair on VU0. An empty channel is skipped, which retains the light's own
     * colour. Rnd::Light::SetColors() receives the three results.
     *
     * @param flFrame The filtered frame to animate to.
     * @ghidraAddress 0x005418f8
     */
    virtual void SetFrameSelf(float flFrame);

private:
    // Empty the three channels unless this animation owns its keys. SetKeysOwner() inlines it.
    // 0x00545570.
    void ClearKeys();

    // No class derives from Rnd::LightAnim and no access from outside it is recovered. Every
    // member is therefore private. The order below is the recovered offset order, and the text
    // dump pins all five.

    // Light this animation drives. SetFrameSelf() returns without doing anything while it is null.
    Light *mLight; // +0x18
    // Channel that drives the ambient colour.
    std::list<ColorKey> mAmbientKeys; // +0x1c
    // Channel that drives the diffuse colour.
    std::list<ColorKey> mDiffuseKeys; // +0x20
    // Channel that drives the specular colour.
    std::list<ColorKey> mSpecularKeys; // +0x24
    // Animation whose channels this one reads, itself for an animation that owns its keys.
    LightAnim *mKeysOwner; // +0x28
};

/**
 * Registered class name of Rnd::LightAnim, the string "LightAnim".
 *
 * @ghidraAddress 0x00720bd8
 */
extern HxStr g_lightAnimClassName;

/**
 * Allocate and construct a light animation.
 *
 * The allocation is 0x48 untagged bytes. The out-of-line copy has no caller, and
 * CreateRegisteredLightAnim() expands the same body.
 *
 * @param name The object name.
 * @return The new animation.
 * @ghidraAddress 0x005449f8
 */
LightAnim *NewLightAnim(const HxStr &name);

/**
 * Build a light animation for the registered "LightAnim" class.
 *
 * Builds as NewLightAnim() does and converts the result to its Rnd::Object virtual base. The class
 * installs no creator hook, unlike Rnd::Light. There is one creator and no platform subclass.
 *
 * @param name The object name.
 * @return The new animation, as its Rnd::Object subobject.
 * @ghidraAddress 0x005452d0
 */
Object *CreateRegisteredLightAnim(const HxStr &name);

/**
 * Register the "LightAnim" class with Rnd::Manager, through CreateRegisteredLightAnim().
 *
 * The out-of-line copy has no caller, and Rnd::Manager::Init() expands the same registration. The
 * name is inferred.
 *
 * @ghidraAddress 0x005449c8
 */
void RegisterLightAnimClass();

} // namespace Rnd
