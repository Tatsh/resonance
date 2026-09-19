#pragma once

#include <list>

#include "math/color.h"
#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/drawable.h"
#include "rnd/light.h"
#include "rnd/object.h"
#include "rnd/stream.h"

namespace Rnd {

/**
 * How fog attenuates a drawn pixel.
 *
 * The seven names come from the routine at `0x00519470`, which writes one of "None", "VertExp",
 * "VertExp2", "VertLinear", "PixelExp", "PixelExp2", and "PixelLinear" for the values below and
 * "out of memory in" for any other. The PlayStation 2 renderer distinguishes none of the six
 * enabled modes. Rnd::PsEnviron::DrawSelf() tests only whether the mode is None.
 */
enum FogMode {
    kFogModeNone = 0,
    kFogModeVertExp = 1,
    kFogModeVertExp2 = 2,
    kFogModeVertLinear = 3,
    kFogModePixelExp = 4,
    kFogModePixelExp2 = 5,
    kFogModePixelLinear = 6
};

/**
 * Lighting and fog a subtree is drawn under.
 *
 * `Q23Rnd7Environ` in the RTTI descriptor at `0x008ef500`, with `Rnd::Drawable` as its only public
 * non-virtual base at offset 0. The class is 0x80 bytes, which the allocator at `0x00519308`
 * proves by requesting exactly that much under the tag "Rnd::Environ". The virtual `Rnd::Object`
 * subobject sits at `0x60`, which the constructor proves by writing that address into the
 * virtual-base pointer, and the four bytes after it are tail padding.
 *
 * Both colour members are quadword aligned in the original, which is what produces the reserved
 * runs at `0x18` and `0x3c`.
 *
 * The member titles come from the text DumpText() writes: "lights:", "ambient:", " fogStart:",
 * "fogEnd:", " fogDensity:", "fogColor:", and " fogMode:".
 *
 * Two vtables belong to the class. The four-entry table at `0x00826d28` is addressed by the
 * `Rnd::Drawable` vptr at `0x10` and declares no virtual of its own, and the eight-entry table at
 * `0x00826ce0` is addressed by the `Rnd::Object` subobject vptr with a `-0x60` adjustment on every
 * entry.
 *
 * Two routines of this class are recovered and not declared. The one at `0x00519470` writes the
 * name of a fog mode to a diagnostic sink, and the one at `0x005185b0` writes mLights to one. Both
 * exist only to serve DumpText(). Nothing in the image separates a static member from a free
 * function for one or for the other.
 */
class Environ : public Drawable {
public:
    /**
     * Construct an environment with no lights and no fog.
     *
     * The ambient colour starts fully transparent black, the fog colour fully opaque white, the
     * fog start at 0.0, the fog end and the fog density at 1.0, and the fog mode at None.
     *
     * @param name The registry key for this object.
     * @ghidraAddress 0x00515890
     */
    explicit Environ(const HxStr &name);

    /**
     * @ghidraAddress 0x00518fe0
     */
    virtual ~Environ();

    /**
     * Report the class key a `.rnd` file writes for an environment.
     *
     * The returned string is the global at `0x00718d18`, which the class registration fills with
     * "Environ".
     *
     * @return The class key.
     * @ghidraAddress 0x00519258
     */
    virtual const HxStr &ClassName() const;

    /**
     * Write a description of this environment to sink.
     *
     * The base description comes first, and everything below is produced only at a positive dump
     * level.
     *
     * @param sink The diagnostic sink to write to.
     * @ghidraAddress 0x00515ce0
     */
    virtual void DumpText(FailSink &sink);

    virtual void Save(Stream &stream);
    virtual void Replace(Object *pFrom, Object *pTo);
    virtual void Copy(const Object *pSource, unsigned nFlags);
    virtual void Load(Stream &stream);

    // Declared in recovered offset order. Every member is public because
    // Rnd::PsEnviron::DrawSelf() reads mLights, mFogMode, the two fog distances, and mFogColor
    // directly, and the image exposes no accessor for any of them.

    /** Lights this environment applies to the subtree below it. +0x14 */
    std::list<Light *> mLights;

private:
    // A reserved run records a span that has not been recovered and is not a field. This one is
    // consistent with the padding before a quadword-aligned colour.
    unsigned char mReserved18[0x08];

public:
    /** Light every vertex receives regardless of the lights above. +0x20 */
    Color mAmbient;

    /** Distance fog starts at. +0x30 */
    float mFogStart;

    /** Distance fog arrives at its full strength. +0x34 */
    float mFogEnd;

    /**
     * Strength the exponential modes raise fog to.
     *
     * The PlayStation 2 renderer does not read it. +0x38
     */
    float mFogDensity;

private:
    unsigned char mReserved3c[0x04];

public:
    /** Colour a fully fogged pixel becomes. +0x40 */
    Color mFogColor;

    /** Which fog model applies, or None for no fog at all. +0x50 */
    FogMode mFogMode;

private:
    unsigned char mReserved54[0x0c];

protected:
    /**
     * Make this environment the one the subtree below is drawn under.
     *
     * Vtable slot 3 of the Rnd::Drawable table. Rnd::PsEnviron overrides it with the routine that
     * also programs the fog registers and builds the light records.
     *
     * @return Non-zero, which draws the children as well.
     * @ghidraAddress 0x00518e60
     */
    virtual int DrawSelf();
};

/**
 * Environment the subtree being drawn is under.
 *
 * Rnd::Environ::DrawSelf() stores itself here, and Rnd::PsEnviron::DrawSelf() stores its own
 * environment the same way.
 *
 * @ghidraAddress 0x00718d20
 */
extern Environ *g_pCurrentEnviron;

} // namespace Rnd
