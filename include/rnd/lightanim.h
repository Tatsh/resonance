#pragma once

#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/light.h"
#include "rnd/object.h"
#include "rnd/stream.h"

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
 * order. Each channel is one word storing the sentinel of a `std::list`, the same arrangement
 * Rnd::MatAnim and Rnd::ParticleSysAnim use, and the shared channel dump at `0x004d8de8` serves
 * all three classes.
 *
 * Keys are shared rather than copied, the same arrangement Rnd::MeshAnim uses with mKeysOwner.
 * Both EndFrame() and SetFrameSelf() read the channels of mKeysOwner rather than their own. An
 * animation whose keys belong to another animation therefore reads that object's keys.
 *
 * The element of a colour channel is now pinned, which it was not when Rnd::MatAnim and
 * Rnd::ParticleSysAnim were written. Three independent readings agree. The node is 0x30 bytes,
 * from the pool slot at `+0x14` of the node pool at `0x00667080` that the channel clear at
 * `0x004dd7d0` returns nodes to. The copy assignment at `0x004d9aa0` moves two quadwords from
 * `+0x10` and `+0x20` of the node, which makes the element 0x20 bytes starting at `+0x10`.
 * SetFrameSelf() loads a quadword from element `+0x00` as the colour and a float from element
 * `+0x10` as the frame, and EndFrame() reads the same float. The element is therefore a colour
 * followed by a frame, with 0xc bytes after the frame that nothing in the image reads. The channel
 * type in this header stays one word until include/rnd/keychannel.h adopts that element, because
 * the shared dump there takes the sentinel word.
 */
class LightAnim : public Animatable {
public:
    /**
     * Construct an animation with three empty channels and no light.
     *
     * The body is not reconstructed. The creator at `0x005452d0` is the one construction site and
     * allocates 0x48 untagged bytes before invoking it. A second identical copy of that creator
     * sits at `0x005449f8`.
     *
     * @param name The object name, passed to the Rnd::Object constructor.
     * @ghidraAddress 0x00544df0
     */
    explicit LightAnim(const HxStr &name);

    /** @ghidraAddress 0x00544b10 */
    virtual ~LightAnim();

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
     * The body is not reconstructed. The revision is 0, the base and the name of mLight follow,
     * then the three channels and the name of mKeysOwner. Writing a channel needs the shared
     * channel writer, which include/rnd/keychannel.h does not declare.
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
     * The body is not reconstructed for the same reason as Save().
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
     * Rnd::Animatable vtable slot 1. The body is not reconstructed. Each channel of mKeysOwner
     * contributes the frame of its last key, or zero while it is empty, and the result is the
     * largest of the three through two nested `std::max` calls, which the pair of selected stack
     * addresses rather than selected values proves. Expressing the walk needs the channel element,
     * which this header does not model; see the class documentation.
     *
     * @return The last frame.
     * @ghidraAddress 0x00541790
     */
    virtual float EndFrame();

protected:
    /**
     * Animate the light to a frame.
     *
     * Rnd::Animatable vtable slot 3. The body is not reconstructed for the same reason as
     * EndFrame(). It returns at once while mLight is null. Otherwise it seeds three local colours
     * from the current ambient, diffuse, and specular of the light, then replaces each from its
     * channel of mKeysOwner. A frame at or before the first key takes that key, a frame at or
     * after the last key takes that key, and a frame between two keys takes the linear blend of
     * the pair on VU0. An empty channel is skipped, which retains the light's own colour.
     * Rnd::Light::SetColors() receives the three results.
     *
     * @param flFrame The filtered frame to animate to.
     * @ghidraAddress 0x005418f8
     */
    virtual void SetFrameSelf(float flFrame);

private:
    // No class derives from Rnd::LightAnim and no access from outside it is recovered. Every
    // member is therefore private. The order below is the recovered offset order, and the text
    // dump pins all five.

    // Light this animation drives. SetFrameSelf() returns without doing anything while it is null.
    Light *mLight; // +0x18
    // Sentinel of the channel that drives the ambient colour.
    int mAmbientKeys; // +0x1c
    // Sentinel of the channel that drives the diffuse colour.
    int mDiffuseKeys; // +0x20
    // Sentinel of the channel that drives the specular colour.
    int mSpecularKeys; // +0x24
    // Animation whose channels this one reads, itself for an animation that owns its keys.
    LightAnim *mKeysOwner; // +0x28
};

/**
 * Read a channel of colour keyframes from a `.rnd` stream.
 *
 * Reads the key count, resizes the channel to it through `std::list::assign` at `0x004d9570`, and
 * then reads one element per node through the element reader at `0x004d9658`.
 *
 * The home of this declaration is include/rnd/keychannel.h alongside the two channel dumps, which
 * Rnd::MatAnim, Rnd::LightAnim, and Rnd::ParticleSysAnim all share. It sits here because that file
 * belongs to another part of the tree.
 *
 * @param stream The stream to read from.
 * @param nChannel The channel, a `std::list` sentinel stored in one word.
 * @return The stream.
 * @ghidraAddress 0x004dd9b0
 */
Stream &ReadColorKeys(Stream &stream, int &nChannel);

/**
 * Empty a channel of colour keyframes.
 *
 * Returns every node to the 0x30-byte slot of the node pool at `0x00667080` and then self-links
 * the sentinel. The home of this declaration is include/rnd/keychannel.h for the same reason as
 * ReadColorKeys().
 *
 * @param nChannel The channel, a `std::list` sentinel stored in one word.
 * @ghidraAddress 0x004dd7d0
 */
void ClearColorKeys(int &nChannel);

/**
 * Registered class name of Rnd::LightAnim, the string "LightAnim".
 *
 * @ghidraAddress 0x00720bd8
 */
extern HxStr g_lightAnimClassName;

/**
 * Allocate and construct a light animation.
 *
 * This is the creator the class registers with Rnd::Manager, at `0x005449c8`. The class installs
 * no creator hook, unlike Rnd::Light. There is therefore one creator and no platform
 * subclass.
 *
 * @param name The object name.
 * @return The new animation.
 * @ghidraAddress 0x005452d0
 */
LightAnim *NewLightAnim(const HxStr &name);

} // namespace Rnd
