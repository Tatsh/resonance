#pragma once

#include <cstddef>
#include <list>

#include "math/color.h"
#include "rnd/drawable.h"
#include "rnd/manager.h"

class FailSink;
class HxStr;
namespace Rnd {
class Light;
class Object;
class Stream;
} // namespace Rnd

namespace Rnd {

/**
 * How fog attenuates a drawn pixel.
 *
 * The seven names come from the routine at `0x00519470`, which writes one of "None", "VertExp",
 * "VertExp2", "VertLinear", "PixelExp", "PixelExp2", and "PixelLinear" for the values below and
 * nothing at all for any other. The PlayStation 2 renderer distinguishes none of the six
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
 * Two routines that serve DumpText() are free functions rather than members, because each returns
 * the sink it was handed. The one at `0x00519470` writes the name of a fog mode and the one at
 * `0x005185b0` writes mLights, and both are declared beside the implementation rather than here
 * because no code outside `environ.cpp` calls either.
 *
 * The class has the tagged allocation pair every class in this tree gets from the allocation macro,
 * under the tag "Rnd::Environ". The allocation is inlined into NewEnviron(), its one call site at
 * `0x00519324`, and no out-of-line copy of either half survives. Neither is declared here, for the
 * same reason APalette declares neither: the macro generates them rather than a programmer writing
 * them.
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

    /**
     * Write this environment's serialised form to stream.
     *
     * The revision word is 0, which is also the highest Load() accepts, so the format never
     * changed.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x00516028
     */
    virtual void Save(Stream &stream);

    /**
     * Repoint a light entry when the object it addressed is replaced.
     *
     * An entry that already addresses pTo produces the report "%s already in %s" and is then
     * replaced regardless. A replacement that is not a light empties the entry, and an empty entry
     * is erased from the list rather than retained as a hole.
     *
     * @param pFrom The object going away.
     * @param pTo The object to store instead, or null.
     * @ghidraAddress 0x005156b0
     */
    virtual void Replace(Object *pFrom, Object *pTo);

    /**
     * Copy the state of pSource into this environment.
     *
     * The light list is copied only while bit 0 of nFlags is clear, and there is no disagreement
     * with kCopyChildLists. The two are different bits of one word with opposite polarity. Bit 0 is
     * an opt-out for the light list, tested here at `0x00516560` by exclusive-or against one
     * followed by a mask. Bit 9 is an opt-in for the child draw list, tested by
     * Rnd::Drawable::Copy() at `0x00506af0`. This routine passes the same word into the base copy
     * first, so one call tests both.
     *
     * @param pSource The environment to copy from.
     * @param nFlags The set of fields to copy.
     * @ghidraAddress 0x00516560
     */
    virtual void Copy(const Object *pSource, unsigned nFlags);

    /**
     * Read this environment's serialised form from stream.
     *
     * A revision above 0 is rejected with "Can't load new Environ". The fog mode arrives as a
     * plain word and is assigned to the enumeration afterwards.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x00516260
     */
    virtual void Load(Stream &stream);

    /**
     * Build an environment the class registry vends.
     *
     * @param name The registry key for the new environment.
     * @return The new environment.
     * @ghidraAddress 0x00519308
     */
    static Environ *NewEnviron(const HxStr &name);

    /**
     * Allocate an environment block under the tag "Rnd::Environ".
     *
     * @param nSize The block size.
     * @return The block.
     * @ghidraAddress 0x00518df0
     */
    static void *operator new(size_t nSize);

    /**
     * Release a block operator new() allocated.
     *
     * @param pBlock The block.
     * @ghidraAddress 0x00518e10
     */
    static void operator delete(void *pBlock);

    /**
     * Append a light to mLights.
     *
     * A light already in the list is reported to g_failSink as "<light> already in <environment>"
     * and is not added again. Otherwise this environment registers as a referrer of the light,
     * when it is not null, and the light is appended.
     *
     * @param pLight The light to add.
     * @ghidraAddress 0x005166d0
     */
    void AddLight(Light *pLight);

    /**
     * Remove a light from mLights.
     *
     * The first matching entry is erased, and this environment's reference on the light is dropped
     * when the light is not null. A light not in the list is ignored. No call site survives in the
     * shipped program, and the name is inferred from AddLight().
     *
     * @param pLight The light to remove.
     * @ghidraAddress 0x00516850
     */
    void RemoveLight(Light *pLight);

    /**
     * Drop this environment's reference on every light and empty mLights.
     *
     * AppTunnel's constructor calls it on "tunnel.env". The title is inferred.
     *
     * @ghidraAddress 0x00519568
     */
    void ClearLights();

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

private:
    // Registers this environment as a referrer of every mLights entry. Inlined in Load(), Copy(),
    // and the tail of Replace(). The out-of-line copy at 0x00519400 has no caller.
    void AcquireLightsRefs();

    // Drops this environment's registration on every mLights entry. Inlined in Load() and
    // Copy(). The destructors of this class and Rnd::PsEnviron call the out-of-line copy at
    // 0x00519390.
    void ReleaseLightsRefs();
};

/**
 * Factory the registered environment creator dispatches through.
 *
 * Rnd::Manager::Init() fills it with Environ::NewEnviron() and Rnd::PsEnviron::Init() replaces it
 * with Rnd::PsEnviron::NewEnviron(), which is how the PlayStation 2 layer substitutes its subclass
 * under the unchanged class key.
 *
 * @ghidraAddress 0x00718d10
 */
extern Environ *(*g_pfnNewEnviron)(const HxStr &name);

/**
 * Build an environment for the registered "Environ" class.
 *
 * Calls through g_pfnNewEnviron and converts the result to its Rnd::Object virtual base, reading
 * the base pointer only when the environment is not null.
 *
 * @param name The object name.
 * @return The new environment, as its Rnd::Object subobject.
 * @ghidraAddress 0x00519278
 */
Object *CreateRegisteredEnviron(const HxStr &name);

/**
 * Build an environment through the creator hook.
 *
 * No call site survives in the shipped program. The name is inferred from the Rnd::Tex
 * counterpart.
 *
 * @param name The object name.
 * @return The new environment.
 * @ghidraAddress 0x00518eb0
 */
Environ *NewEnvironThroughHook(const HxStr &name);

/**
 * Registered class name of Rnd::Environ, the string "Environ".
 *
 * @ghidraAddress 0x00718d18
 */
extern HxStr g_environClassName;

/**
 * Point g_pfnNewEnviron at Environ::NewEnviron() and register the "Environ" class with
 * Rnd::Manager.
 *
 * The out-of-line copy has no caller, and Rnd::PsEnviron::Terminate() expands the body. The name
 * is inferred.
 *
 * @ghidraAddress 0x00518e70
 */
inline void RegisterEnvironClass() {
    g_pfnNewEnviron = Environ::NewEnviron;
    g_manager.RegisterClass(g_environClassName, CreateRegisteredEnviron);
}

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
