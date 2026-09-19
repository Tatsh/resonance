#pragma once

#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/collideable.h"
#include "rnd/drawable.h"
#include "rnd/object.h"
#include "rnd/raytest.h"
#include "rnd/stream.h"

namespace Rnd {

/**
 * Procedural tunnel the game flies the player through.
 *
 * `Q23Rnd6Tunnel` in the RTTI descriptor at `0x008efdb0`, with three public non-virtual bases:
 * `Rnd::Drawable` at `+0x00`, `Rnd::Animatable` at `+0x14`, and `Rnd::Collideable` at `+0x2c`. All
 * three derive virtually from `Rnd::Object`, so one shared Object subobject sits at `+0xe8`. The
 * creator allocates exactly 0x104 bytes, which is `0xe8` plus the 0x1c-byte Object subobject with
 * nothing left over, and the three base sizes of 0x14, 0x18, and 0xc account for everything ahead
 * of `+0x38`. The members of the class itself therefore occupy `+0x38` through `+0xe7`.
 *
 * This is the only render tunnel in the build. The four other `Tnl`-prefixed classes,
 * `TnlArena`, `TnlTrigger`, `TnlPanelFXDelay`, and `AppTunnel`, all derive from `MsgSink` and
 * belong to the game rather than the renderer.
 *
 * Four vtables belong to the class, each identified by its own GetTypeInfo slot addressing
 * `0x004761b0` and by an adjustment matching its subobject offset. The Object subobject table at
 * `0x0081d4f0` adjusts by `-0xe8`, the Collideable table at `0x0081d480` by `-0x2c`, the
 * Animatable table at `0x0081d4a0` by `-0x14`, and the Drawable table at `0x0081d4c8` by zero.
 *
 * Beyond the seven Object virtuals the class overrides exactly three, one in each mix-in:
 * Collide() at Collideable slot 1, SetFrameSelf() at Animatable slot 3, and DrawSelf() at Drawable
 * slot 3. Both remaining Animatable slots and both remaining Drawable slots still address the base
 * implementations.
 *
 * DumpText() is a stub the original author never finished. It emits the three base dumps and then
 * the two literals "[Tunnel]\n" and "TODO\n", and writes no member at all. **No member of this
 * class has a name anywhere in the image**, because the text dump is the only routine that would
 * have supplied one. Every member below is therefore titled by its offset, and the ones that carry
 * a recovered initial value say so; that value is the only evidence about them.
 *
 * Load() accepts a bounded version range rather than an upper bound alone, which is the only class
 * in the renderer that does. It reports "Can't load new Tunnel" above the range and "Can't load
 * old Tunnel" below it.
 *
 * The class builds named child objects as it generates geometry, through the formats "%s.%d",
 * "[%s.%d]", "%s_lat%03d", "%s_pan%03d", and "[%s_seek%d.%d]". The lateral and pan names suggest
 * one child mesh per axis per section and the seek name a per-section marker, although no routine
 * that consumes them is reconstructed yet.
 *
 * Recovery is partial, and this header is the structural pass. Nothing that walks the geometry is
 * reconstructed. The routines already identified are the constructor at `0x00466620`, the face
 * strip append at `0x00466528`, the update at `0x00467f48`, the section node vector setup at
 * `0x004694a0` and `0x00476190`, the section bounding box resize at `0x004698e8` and `0x00473538`,
 * the mesh build at `0x004699c0`, the material section list clear at `0x0046acf0`, the face strip
 * build at `0x0046adc0` and `0x00476598`, the VU1 upload at `0x0046c0e8`, the section frame setter
 * at `0x0046c638`, the ring mesh finalise at `0x0046d180`, the camera space projection at
 * `0x0046db80`, the drawable index table rebuild at `0x00472488`, the section node vector extend at
 * `0x00474990`, and the section mesh VU1 upload at `0x00494048`.
 */
class Tunnel : public Drawable, public Animatable, public Collideable {
public:
    /**
     * Construct a tunnel with no sections.
     *
     * The body is not reconstructed. It writes the initial values recorded on the members below
     * and allocates the sentinel node of each list member.
     *
     * @param name The object name, passed to the Rnd::Object constructor.
     * @ghidraAddress 0x00466620
     */
    Tunnel(const HxStr &name);

    /** @ghidraAddress 0x004676b0 */
    virtual ~Tunnel();

    /**
     * Write the tunnel to the engine text sink.
     *
     * Emits the Object, Drawable, and Animatable dumps, then the "[Tunnel]" block, which is the
     * literal "TODO" and nothing else. The Collideable dump is not emitted. The block is
     * suppressed while the dump level of the sink is not positive.
     *
     * @param sink The text sink.
     * @ghidraAddress 0x004768b8
     */
    virtual void DumpText(FailSink &sink);

    /**
     * Serialise the tunnel.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x004682a8
     */
    virtual void Save(Stream &stream);

    /**
     * Replace one object reference with another.
     *
     * @param pFrom The object being replaced.
     * @param pTo The object to point at, which may be null.
     * @ghidraAddress 0x004680e0
     */
    virtual void Replace(Object *pFrom, Object *pTo);

    /**
     * Return the registered class name, "Tunnel".
     *
     * @return The class name.
     * @ghidraAddress 0x004763b8
     */
    virtual const HxStr &ClassName() const;

    /**
     * Copy another tunnel over this one.
     *
     * @param pSource The source object, which has to be a tunnel for the copy to have any effect.
     * @param nFlags The copy flags.
     * @ghidraAddress 0x00476788
     */
    virtual void Copy(const Object *pSource, unsigned nFlags);

    /**
     * Load the tunnel.
     *
     * Rejects a file outside the version range it accepts, reporting "Can't load new Tunnel" above
     * it and "Can't load old Tunnel" below it.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x00468538
     */
    virtual void Load(Stream &stream);

    /**
     * Test a ray against the tunnel and append what it strikes to sink.
     *
     * Rnd::Collideable vtable slot 1.
     *
     * @param ray The segment to test along.
     * @param sink The collector to append intersections to.
     * @ghidraAddress 0x00476a10
     */
    virtual void Collide(const Ray &ray, HitSink &sink);

    /**
     * Regenerate the geometry that the current state calls for.
     *
     * @ghidraAddress 0x00467f48
     */
    void Update();

protected:
    /**
     * Draw the tunnel.
     *
     * Rnd::Drawable vtable slot 3.
     *
     * @return Non-zero when the children are to be drawn as well.
     * @ghidraAddress 0x00468850
     */
    virtual int DrawSelf();

    /**
     * Advance the tunnel to a frame.
     *
     * Rnd::Animatable vtable slot 3.
     *
     * @param flFrame The filtered frame to animate to.
     * @ghidraAddress 0x00469180
     */
    virtual void SetFrameSelf(float flFrame);

private:
    // Upload the generated geometry to VU1. 0x0046c0e8.
    void UploadToVU1();

    // Build the mesh of the current ring set. 0x004699c0.
    void BuildMesh();

    // Close the mesh of one ring. 0x0046d180.
    void FinalizeRingMesh();

    // Project one section into camera space. 0x0046db80.
    void ProjectSectionToCameraSpace();

    // Rebuild the table the draw path indexes its children through. 0x00472488.
    void RebuildDrawableIndexTable();

    // Empty the per-material section lists. 0x0046acf0.
    void ClearMaterialSectionLists();

    // Set the frame of every section of one ring. 0x0046c638.
    void SetRingSectionFrames();

    // No class derives from Rnd::Tunnel and no access from outside it is recovered, so every
    // member is private. The order below is the recovered offset order. Every title is the offset
    // itself, because DumpText() is a stub and no other routine in the image identifies a member
    // by name. The initial value each one receives from the constructor is the whole of the
    // evidence about it, and the members with no recorded value are the ones the constructor
    // leaves alone or fills through a list allocation.

    float mUnknown38;               // +0x38 Starts at 1.0f.
    int mUnknown3c;                 // +0x3c Starts at 3.
    int mUnknown40;                 // +0x40 Starts at 0.
    int mUnknown44;                 // +0x44 Starts at 2.
    float mUnknown48;               // +0x48 Starts at 0.099609375f.
    float mUnknown4c;               // +0x4c Starts at 0.099609375f.
    float mUnknown50;               // +0x50 Starts at 0.25f.
    float mUnknown54;               // +0x54 Starts at 0.01f.
    int mUnknown58;                 // +0x58 Starts at 0.
    int mUnknown5c;                 // +0x5c Starts at 0.
    int mUnknown60;                 // +0x60 Starts at 0.
    float mUnknown64;               // +0x64 Starts at 480.0f, which is the display height.
    int mUnknown68;                 // +0x68 Starts at 0.
    unsigned char mUnknown6c[0x08]; // +0x6c Unrecovered.
    int mUnknown74;                 // +0x74 Starts at 1.
    int mUnknown78;                 // +0x78 Starts at 1.
    // Starts at 99999999, which is a hand-written sentinel in the same style as the -9999999.0f
    // Rnd::ParticleSys uses for an unset frame.
    int mUnknown7c;                 // +0x7c
    int mUnknown80;                 // +0x80 Starts at 0.
    int mUnknown84;                 // +0x84 Starts at 0.
    int mUnknown88;                 // +0x88 Starts at 0.
    unsigned char mUnknown8c[0x0c]; // +0x8c Unrecovered.
    int mUnknown98;                 // +0x98 Starts at 0.
    int mUnknown9c;                 // +0x9c Starts at 0.
    int mUnknowna0;                 // +0xa0 Unrecovered.
    int mUnknowna4;                 // +0xa4 Starts at 0.
    unsigned char mUnknowna8[0x08]; // +0xa8 Unrecovered.
    int mUnknownb0;                 // +0xb0 Starts at 0.
    unsigned char mUnknownb4[0x08]; // +0xb4 Unrecovered.
    int mUnknownbc;                 // +0xbc Starts at 0.
    int mUnknownc0;                 // +0xc0 Starts at 0.
    unsigned char mUnknownc4[0x08]; // +0xc4 Unrecovered.
    int mUnknowncc;                 // +0xcc Starts at 0.
    unsigned char mUnknownd0[0x0c]; // +0xd0 Unrecovered.
    int mUnknowndc;                 // +0xdc Starts at 0.
    unsigned char mUnknowne0[0x08]; // +0xe0 Unrecovered.
};

/**
 * Allocate and construct a tunnel.
 *
 * This is the creator the class registers with Rnd::Manager. The class installs no creator hook,
 * so there is one creator and no platform subclass.
 *
 * @param name The object name.
 * @return The new tunnel.
 * @ghidraAddress 0x00476288
 */
Tunnel *NewTunnel(const HxStr &name);

/**
 * Registered class name of Rnd::Tunnel, the string "Tunnel".
 *
 * @ghidraAddress 0x006eab10
 */
extern HxStr g_tunnelClassName;

} // namespace Rnd
