#pragma once

#include <list>
#include <vector>

#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/keychannel.h"

class FailSink;
namespace Rnd {
class Mat;
class Object;
class Stream;
class Tex;
} // namespace Rnd

namespace Rnd {

/**
 * Animation of the colours and the texture stages of one material.
 *
 * `Q23Rnd7MatAnim` in the RTTI descriptor at `0x008ef4b0`, with `Rnd::Animatable` as its one
 * public base at offset 0. The Animatable subobject is 0x18 bytes, which its constructor pins from
 * inside by storing the primary vtable pointer at `+0x14` and the two Animatable lists at `+0x04`
 * and `+0x08`. The members below therefore start at `+0x18`, the MatAnim subobject is 0x40 bytes,
 * and the shared Rnd::Object subobject sits at `+0x40`. The creator at `0x004dcb00` allocates
 * exactly 0x5c bytes, which is that 0x40 plus the 0x1c of the Object subobject with nothing left
 * over, and the `-0x40` adjustment on every entry of the Object subobject table confirms the
 * offset from outside.
 *
 * Two vtables belong to the class, each with the type function at `0x004dbf10` in slot 0. The
 * Object subobject table at `0x00822d68` stores the destructor and the seven Object overrides, and
 * the primary table at `0x00822db0` stores EndFrame() at slot 1 and SetFrameSelf(float) at slot 3,
 * followed by an all-zero terminator. Slot 2 still addresses the base Rnd::Animatable::StartAnim()
 * at `0x0049a3b8`, so restarting an animation does nothing of its own here.
 *
 * Five channels animate the four material colours and the alpha, and the stage vector animates the
 * texture stages one for one against the material's own stages. Two independent readings agree on
 * which colour each channel drives. The text dump titles them " diffuseKeys:", "ambientKeys",
 * "emissiveKeys: ", "specularKeys:", and "alphaKeys:" in offset order. And SetFrameSelf() hands
 * each interpolated result to the material through vtable slot 10, slot 9, slot 11, slot 13, and
 * slot 12 respectively, which are Rnd::Mat::SetDiffuse(), SetAmbient(), SetEmissive(),
 * SetSpecular(), and SetAlpha() in the declaration order of include/rnd/mat.h. The two readings
 * agreeing matters, because an earlier pass had mDiffuseKeys and mEmissiveKeys the other way
 * round.
 *
 * Keys are shared rather than copied, the same arrangement Rnd::MeshAnim uses with mKeysOwner.
 * EndFrame() and SetFrameSelf() read the channels of mKeysOwner rather than their own.
 */
class MatAnim : public Animatable {
public:
    /** Revision Save() writes, and the highest revision Load() accepts. */
    enum { kSerialVersion = 2 };

    /**
     * Bit of the copy flags that shares the source's keyframe channels rather than copying them.
     *
     * Recovered from the `andi` at `0x004dd404` in Copy(). Rnd::MeshAnim reads bit 0x40,
     * Rnd::LightAnim bit 0x02, and Rnd::ParticleSysAnim bit 0x80 for the same purpose, so the bit
     * is per class rather than shared across the hierarchy.
     */
    enum { kCopyShareKeys = 0x04 };

    /**
     * Animation of one texture stage of the material.
     *
     * The record is 0x14 bytes, which the stride of every walk over the vector pins, and the
     * walk in SetFrameSelf() runs it against `mMat->mStages` with the material's stage count as
     * the bound. The three vector channels blend three components rather than four, through
     * `vmulax.xyz` instead of `vmulax.xyzw`, while their keyframe record is the same 0x20 bytes
     * with its frame at `+0x10` that Rnd::ColorKey has. Whether the element is Rnd::ColorKey or a
     * distinct three-component record is therefore undetermined, and Rnd::ColorKey is used here
     * because every measurable property agrees with it.
     *
     * The record has behaviour, so it is a class with private members rather than a plain data
     * record. Rnd::MatAnim is the only code that touches the four channels, through the nested
     * access a member of the enclosing class has.
     */
    class StageAnim {
    public:
        /**
         * One keyframe of the texture channel of a stage.
         *
         * The record is 8 bytes. The reference walk at `0x004d3750` takes a reference on list node
         * `+0x08`, which is the payload of an 8-byte element, and the end-frame walk at
         * `0x004d42f0` reads the frame at node `+0x0c`. The title is inferred on the same basis as
         * Rnd::ColorKey.
         */
        struct TexKey {
            Tex *mValue;  /*!< Texture the frame switches to. +0x00 */
            float mFrame; /*!< Frame the texture applies at. +0x04 */
        };

        /**
         * Serialise the stage animation.
         *
         * The body is not reconstructed. Rnd::MatAnim::Save() writes the vector through this one,
         * once per element, and the call passes the stage as the object and the stream as the one
         * argument.
         *
         * @param stream The stream to write to.
         * @ghidraAddress 0x004dd500
         */
        void Save(Stream &stream);

    private:
        friend class MatAnim;

        // Channel blended into the last row of the stage transform, which is its translation.
        std::list<ColorKey> mTranslateKeys; // +0x00
        // Channel blended and then applied to the stage transform through `0x0045da58`.
        std::list<ColorKey> mXfmKeys; // +0x04
        // Channel blended and then applied to the stage transform through `0x004f0430`.
        std::list<ColorKey> mRotateKeys; // +0x08
        // Channel of texture references. Its element is 8 bytes, a texture at `+0x00` and the
        // frame at `+0x04`, which the reference walk at `0x004d3750` pins by taking a reference on
        // list node `+0x08` and the end-frame walk at `0x004d42f0` by reading the frame at node
        // `+0x0c`.
        std::list<TexKey> mTexKeys; // +0x0c
        // Animation this stage record belongs to. `0x004d3818` writes `this` through it while
        // taking the references of the whole animation.
        MatAnim *mOwner; // +0x10
    };

    /**
     * Construct an animation with five empty channels, no stages, and no material.
     *
     * The body is not reconstructed. Rnd::CreateRegisteredMatAnim() is the one construction site.
     *
     * @param name The object name, passed to the Rnd::Object constructor.
     * @ghidraAddress 0x004dc4f0
     */
    MatAnim(const HxStr &name);

    /** @ghidraAddress 0x004dc0c8 */
    virtual ~MatAnim();

    /**
     * Write the animation to the engine text sink.
     *
     * The body is not reconstructed. Emits the Rnd::Object and Rnd::Animatable dumps, then the
     * "[MatAnim]" block. The block is suppressed while the dump level of the sink is not positive.
     * Writing the stage block needs the stage vector dump at `0x004d8b78`.
     *
     * @param sink The text sink.
     * @ghidraAddress 0x004d33b8
     */
    virtual void DumpText(FailSink &sink);

    /**
     * Serialise the animation.
     *
     * Writes kSerialVersion, the Rnd::Animatable subobject, mMat as a name, the stage vector,
     * mKeysOwner as a name, and finally the five channels.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x004d35d0
     */
    virtual void Save(Stream &stream);

    /**
     * Replace one object reference with another.
     *
     * The body is not reconstructed.
     *
     * @param pFrom The object being replaced.
     * @param pTo The replacement, which may be null.
     * @ghidraAddress 0x004d30c8
     */
    virtual void Replace(Object *pFrom, Object *pTo);

    /**
     * Report the registered class name, "MatAnim".
     *
     * @return The class name.
     * @ghidraAddress 0x004dc4e0
     */
    virtual const HxStr &ClassName() const;

    /**
     * Copy another animation over this one.
     *
     * kCopyShareKeys shares the source's channels instead of copying them, and a source that is
     * itself sharing is always shared from rather than copied. Unlike its three sibling classes
     * this one has no `mKeysOwner != this` test around the clear, because ClearKeys() performs
     * that test itself.
     *
     * @param pSource The object to copy from.
     * @param nFlags The set of fields to copy.
     * @ghidraAddress 0x004dd388
     */
    virtual void Copy(const Object *pSource, unsigned nFlags);

    /**
     * Load the animation.
     *
     * The body is not reconstructed.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x004d38e8
     */
    virtual void Load(Stream &stream);

    /**
     * Report the last frame this animation runs to.
     *
     * Rnd::Animatable vtable slot 1. Every channel of every stage of mKeysOwner and then each of
     * the five colour and alpha channels contributes the frame of its last key, or zero while it
     * is empty, and the result is the largest across all of them. The walk threads one running
     * maximum through nested `std::max` calls, and the selected stack addresses rather than
     * selected values prove that each pair is a call.
     *
     * @return The last frame.
     * @ghidraAddress 0x004d42f0
     */
    virtual float EndFrame();

protected:
    /**
     * Apply the animation at a frame.
     *
     * Rnd::Animatable vtable slot 3. The body is unrecovered. The stage half walks `mMat->mStages`
     * and writes the stage transform through the two helpers at `0x0045da58` and `0x004f0430`,
     * neither of which is recovered.
     *
     * The colour half is no longer blocked. Rnd::Mat::SetDiffuse() and Rnd::Mat::SetSpecular() now
     * take a Color, which the four quadword blends of this routine settled against the three
     * three-component blends of its stage half.
     *
     * Everything else about it is recovered. The specular call passes an alpha of zero, which
     * `clear f12` at `0x004d54ec` proves, and the alpha channel interpolates linearly rather than
     * on VU0.
     *
     * @param flFrame The frame to apply.
     * @ghidraAddress 0x004d4820
     */
    virtual void SetFrameSelf(float flFrame);

private:
    // Empty every channel of this animation, and its stage vector, unless it owns its own keys.
    // The test on mKeysOwner is the first thing the body does. 0x004d2fe0.
    void ClearKeys();

    // Take a reference on the material, on the keys owner, and on every texture of every stage
    // channel, and record this animation in each stage. 0x004d3818.
    void AddObjectRefs();

    // Drop the references AddObjectRefs() took. 0x004d3750.
    void RemoveObjectRefs();

    // No class derives from Rnd::MatAnim and no access from outside it is recovered, so every
    // member is private. The order below is the recovered offset order.

    // The material this animation drives.
    Mat *mMat; // +0x18
    // One stage animation per texture stage of the material.
    std::vector<StageAnim> mStages; // +0x1c
    // Animation whose channels this one reads, itself for an animation that owns its keys.
    MatAnim *mKeysOwner; // +0x28
    // Channel that drives Rnd::Mat::SetDiffuse().
    std::list<ColorKey> mDiffuseKeys; // +0x2c
    // Channel that drives Rnd::Mat::SetAmbient().
    std::list<ColorKey> mAmbientKeys; // +0x30
    // Channel that drives Rnd::Mat::SetEmissive().
    std::list<ColorKey> mEmissiveKeys; // +0x34
    // Channel that drives Rnd::Mat::SetSpecular(), always with an alpha of zero.
    std::list<ColorKey> mSpecularKeys; // +0x38
    // Channel that drives Rnd::Mat::SetAlpha().
    std::list<FloatKey> mAlphaKeys; // +0x3c
};

/**
 * Allocate and construct a material animation for the registered "MatAnim" class.
 *
 * Allocates 0x5c bytes and returns the Rnd::Object subobject of the new animation, the same shape
 * Rnd::CreateRegisteredMeshAnim() has.
 *
 * @param name The object name.
 * @return The new animation, as its Rnd::Object subobject.
 * @ghidraAddress 0x004dcb00
 */
Object *CreateRegisteredMatAnim(const HxStr &name);

/**
 * Registered class name of Rnd::MatAnim, the string "MatAnim".
 *
 * A static constructor fills the string from the literal at `0x00822e68`.
 *
 * @ghidraAddress 0x00700428
 */
extern HxStr g_matAnimClassName;

} // namespace Rnd
