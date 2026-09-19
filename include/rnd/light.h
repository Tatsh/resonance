#pragma once

#include "math/color.h"
#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/object.h"
#include "rnd/stream.h"
#include "rnd/transformable.h"

namespace Rnd {

/**
 * Which lighting model a light applies.
 *
 * The three names come from the routine at `0x005454a0`, which writes "Point", "Directional", and
 * "Spot" for the values below. Rnd::PsEnviron::DrawSelf() handles the first two and skips a spot
 * light entirely.
 */
enum LightType { kLightTypePoint = 0, kLightTypeDirectional = 1, kLightTypeSpot = 2 };

/**
 * Light in a scene.
 *
 * `Q23Rnd5Light` in the RTTI descriptor at `0x008efe60`, with `Rnd::Transformable` as its only
 * public non-virtual base at offset 0. The class is 0x120 bytes and the virtual `Rnd::Object`
 * subobject sits at `0x100`, which the constructor proves by writing that address into the
 * virtual-base pointer. The four bytes between the last member and that subobject are a reserved
 * run rather than a field.
 *
 * Each colour member is quadword aligned in the original, which is what places the first one at
 * `0xb0` rather than at the `0xac` the base subobject ends on.
 *
 * The member titles come from the text DumpText() writes: "diffuse:", " ambient:", "specular:",
 * " innerAng:", "outerAng:", " range:", "constantAtten:", " linearAtten:", " quadraticAtten:",
 * and "type:".
 *
 * Two vtables belong to the class. The nine-entry table at `0x00829500` is addressed by the
 * `Rnd::Transformable` vptr at `0xa8` and stores the six virtuals declared here after the two
 * `Rnd::Transformable` ones, and the eight-entry table at `0x008294b8` is addressed by the
 * `Rnd::Object` subobject vptr with a `-0x100` adjustment on every entry. Both
 * `Rnd::Transformable` virtuals are inherited unchanged.
 *
 * The position a point light is placed at is the translation row of the world transform of the
 * base, and the direction a directional light points along is the second basis row negated. The
 * light therefore stores neither.
 */
class Light : public Transformable {
public:
    /**
     * Construct a white directional light.
     *
     * The diffuse colour starts fully opaque white, the ambient colour a tenth of that, and the
     * specular colour fully opaque black. Both cone angles start at a right angle halved, the
     * range at 1000.0, the constant attenuation at 1.0, both other attenuations at 0.0, and the
     * type at Directional.
     *
     * @param name The registry key for this object.
     * @ghidraAddress 0x0053ffd8
     */
    explicit Light(const HxStr &name);

    /**
     * @ghidraAddress 0x005445d8
     */
    virtual ~Light();

    /**
     * Set all three colours at once.
     *
     * Vtable slot 3 of the Rnd::Transformable table. The parameter order is the recovered one and
     * it is not the order the text dump writes the three colours in.
     *
     * @param ambient The light every vertex receives from this light regardless of its normal.
     * @param diffuse The light a vertex receives in proportion to its normal.
     * @param specular The highlight colour.
     * @ghidraAddress 0x00544400
     */
    virtual void SetColors(const Color &ambient, const Color &diffuse, const Color &specular);

    /**
     * Set which lighting model this light applies.
     *
     * Vtable slot 4 of the Rnd::Transformable table.
     *
     * @param type The lighting model.
     * @ghidraAddress 0x00544420
     */
    virtual void SetType(LightType type);

    /**
     * Set the distance a point light illuminates over.
     *
     * Vtable slot 5 of the Rnd::Transformable table.
     *
     * @param flRange The distance.
     * @ghidraAddress 0x00544428
     */
    virtual void SetRange(float flRange);

    /**
     * Set the two cone angles of a spot light.
     *
     * Vtable slot 6 of the Rnd::Transformable table.
     *
     * @param flInner Angle the cone is at full strength within.
     * @param flOuter Angle the cone falls to nothing at.
     * @ghidraAddress 0x00544430
     */
    virtual void SetAngles(float flInner, float flOuter);

    /**
     * Set the three attenuation terms of a point light.
     *
     * Vtable slot 7 of the Rnd::Transformable table. The three terms scale one, the distance, and
     * the squared distance.
     *
     * @param flConstant The constant term.
     * @param flLinear The term the distance scales.
     * @param flQuadratic The term the squared distance scales.
     * @ghidraAddress 0x00544440
     */
    virtual void SetAttenuation(float flConstant, float flLinear, float flQuadratic);

    /**
     * Sixth virtual of this class, whose purpose is unrecovered.
     *
     * Vtable slot 8 of the Rnd::Transformable table. The body is empty, no caller is recovered,
     * and no class overrides it. Nothing in the image records what it is for or what it takes,
     * and the signature below is the narrowest one consistent with the body.
     *
     * @ghidraAddress 0x00544818
     */
    virtual void ApplyUnknown();

    /**
     * Report the class key a `.rnd` file writes for a light.
     *
     * The returned string is the global at `0x00720bd0`, which the class registration fills with
     * "Light".
     *
     * @return The class key.
     * @ghidraAddress 0x005445c8
     */
    virtual const HxStr &ClassName() const;

    /**
     * Write a description of this light to sink.
     *
     * The base description comes first, and everything below is produced only at a positive dump
     * level.
     *
     * @param sink The diagnostic sink to write to.
     * @ghidraAddress 0x00540420
     */
    virtual void DumpText(FailSink &sink);

    virtual void Save(Stream &stream);
    virtual void Replace(Object *pFrom, Object *pTo);
    virtual void Copy(const Object *pSource, unsigned nFlags);
    virtual void Load(Stream &stream);

    // Declared in recovered offset order, with the access specifiers interleaved. Each member that
    // remains public is read directly by Rnd::PsEnviron::DrawSelf() while it builds the light
    // records, and the image exposes no accessor for it.

    /** Light a vertex receives in proportion to its normal. +0xb0 */
    Color mDiffuse;

    /** Light every vertex receives from this light regardless of its normal. +0xc0 */
    Color mAmbient;

private:
    Color mSpecular;   // +0xd0
    float mInnerAngle; // +0xe0
    float mOuterAngle; // +0xe4

public:
    /** Distance a point light illuminates over. +0xe8 */
    float mRange;

private:
    float mConstantAtten;  // +0xec
    float mLinearAtten;    // +0xf0
    float mQuadraticAtten; // +0xf4

public:
    /** Which lighting model this light applies. +0xf8 */
    LightType mType;

private:
    // A reserved run records a span that has not been recovered and is not a field. This one is
    // either such a field or the alignment the virtual base subobject at 0x100 is placed on.
    unsigned char mReservedfc[0x04];
};

} // namespace Rnd
