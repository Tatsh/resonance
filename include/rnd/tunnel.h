#pragma once

#include <vector>

#include "math/transform.h"
#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/collideable.h"
#include "rnd/drawable.h"
#include "rnd/mesh.h"
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
 * have supplied one. Every member below is therefore titled by its offset. The ones with a
 * recovered initial value record it, and that value is the only evidence about them.
 *
 * Load() accepts a bounded version range rather than an upper bound alone, which is the only class
 * in the renderer that does. It reports "Can't load new Tunnel" above the range and "Can't load
 * old Tunnel" below it.
 *
 * The class builds child objects with generated titles as it generates geometry, through the
 * formats "%s.%d",
 * "[%s.%d]", "%s_lat%03d", "%s_pan%03d", and "[%s_seek%d.%d]". The lateral and pan names suggest
 * one child mesh per axis per section and the seek name a per-section marker, although no routine
 * that consumes them is reconstructed yet.
 *
 * Recovery is partial, and this header is the structural pass. Nothing that walks the geometry is
 * reconstructed. The members below are now measured rather than only counted, because six small
 * routines index them and their strides and moduli pin the shape of each container. Those six are
 * the ring section lookups at `0x00477388` and `0x004773e8`, the tangent interpolation at
 * `0x00477538`, the section setter at `0x00477830`, the scroll at `0x00476f48`, and the ring
 * advance at `0x00476fe0`. The mesh list clear at `0x0046acf0` confirms the element type of the two
 * grids.
 *
 * Still unreconstructed and still owned by this class are the constructor at `0x00466620`, the
 * update at `0x00467f48`, the section node vector setup at `0x004694a0`, the mesh build at
 * `0x004699c0`, the face strip build at `0x0046adc0`, the VU1 upload at `0x0046c0e8`, the section
 * frame setter at `0x0046c638`, the ring mesh finalise at `0x0046d180`, the camera space
 * projection at `0x0046db80`, and the drawable index table rebuild at `0x00472488`.
 *
 * Seven addresses a worklist grouped under this class belong elsewhere, and each one is recorded
 * here so the grouping is not repeated. `0x00466528` and `0x00476598` append triangles to the face
 * vector a `Rnd::Mesh` addresses through its own mFacesOwner at `+0x134`, which is past the end of
 * this class, so both are Rnd::Mesh members. `0x00493b60` and `0x00493bd0` set mVertsOwner and
 * mFacesOwner of a `Rnd::Mesh` at those same offsets, and `0x00494048` writes a quadword into each
 * 0x40-byte element of a range the object at `+0x130` owns, so all three are Rnd::Mesh members too.
 * `0x00473538` and `0x00474990` measure their range with an arithmetic shift of six, which is a
 * `std::vector` of 0x40-byte elements rather than anything of this class. `0x00476190`,
 * `0x00476a80`, and `0x00476b28` are the default constructor, the destructor, and the element
 * release of a `std::vector`, and `0x00476e48` takes a vector rather than a tunnel as its first
 * argument.
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

    /**
     * Report the first mesh of one slice.
     *
     * The slice index wraps through a signed remainder against mUnknown40, so a negative index
     * addresses from the end.
     *
     * @param nSlice The slice index, taken modulo mUnknown40.
     * @return The first mesh of the slice.
     * @ghidraAddress 0x004773e8
     */
    Mesh *GetRingSection(int nSlice);

    /**
     * Report the first mesh of one ring of one slice.
     *
     * Both indices wrap through a signed remainder, the ring against mUnknown3c and the slice
     * against mUnknown40.
     *
     * @param nRing The ring index, taken modulo mUnknown3c.
     * @param nSlice The slice index, taken modulo mUnknown40.
     * @return The first mesh of the ring.
     * @ghidraAddress 0x00477388
     */
    Mesh *GetRingSection(int nRing, int nSlice);

    /**
     * Blend the translation of one ring transform towards the next.
     *
     * The successor wraps through a signed remainder against mUnknown3c. Only the three components
     * are blended; the padding word of the output is not written.
     *
     * @param nRing The ring index.
     * @param pOut The vector to write the blend into.
     * @param flWeight The weight of the successor, with the complement applied to entry nRing.
     * @ghidraAddress 0x00477538
     */
    void LerpRingSectionTangent(int nRing, Vector3 *pOut, float flWeight);

    /**
     * Bring every slice of the current window up to the advanced slice.
     *
     * Walks the mUnknown40 slices starting at mUnknownbc and calls AdvanceRing() for each one whose
     * mUnknown88 entry differs from its own index.
     *
     * @ghidraAddress 0x00476f48
     */
    void ScrollRings();

    /**
     * Advance one slice and refresh the section frames.
     *
     * A slice that differs from mUnknown7c retires the previous slice by writing the 99999999
     * sentinel into its mUnknown88 entry, then reloads mUnknown80 and mUnknown84.
     * SetRingSectionFrames() runs either way. The step counter then either expires, which records
     * the slice as current, or decrements and accumulates one step into mUnknown80.
     *
     * @param nSlice The slice to advance to.
     * @ghidraAddress 0x00476fe0
     */
    void AdvanceRing(int nSlice);

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
    // evidence about it. A member with no recorded value is one the constructor does not write, or
    // one it fills through a container allocation.

    float mUnknown38; // +0x38 Starts at 1.0f.
    // +0x3c Starts at 3. The ring count. It is the modulus of the inner index of mUnknowna4, the
    // modulus of the index of mUnknownc0, and the row stride of mUnknowna4.
    int mUnknown3c;
    // +0x40 Starts at 0. The slice count. It is the modulus of the index of mUnknownb0, the
    // modulus of the outer index of mUnknowna4, and the length of the array mUnknown88 addresses.
    int mUnknown40;
    int mUnknown44;   // +0x44 Starts at 2.
    float mUnknown48; // +0x48 Starts at 0.099609375f.
    float mUnknown4c; // +0x4c Starts at 0.099609375f.
    float mUnknown50; // +0x50 Starts at 0.25f.
    float mUnknown54; // +0x54 Starts at 0.01f.
    int mUnknown58;   // +0x58 Starts at 0.
    int mUnknown5c;   // +0x5c Starts at 0.
    int mUnknown60;   // +0x60 Starts at 0.
    float mUnknown64; // +0x64 Starts at 480.0f, which is the display height.
    // +0x68 Starts at 0. An object whose first word is its virtual Rnd::Object base pointer, which
    // is how the setter at 0x00477830 registers this tunnel as a referrer of it. The static type is
    // therefore a class deriving virtually from Rnd::Object rather than Rnd::Object itself, and no
    // routine narrows it further, so it stays an offset-titled word.
    int mUnknown68;
    // +0x6c The object the setter at 0x00477830 stores, registered the same way as mUnknown68. Its
    // own `+0x24` supplies mUnknown74.
    int mUnknown6c;
    // +0x70 Unrecovered. No routine reads or writes it.
    int mUnknown70;
    float mUnknown74; // +0x74 Starts at 1. The setter copies it from `+0x24` of mUnknown6c.
    int mUnknown78;   // +0x78 Starts at 1.
    // +0x7c Starts at 99999999, which is a hand-written sentinel in the same style as the
    // -9999999.0f Rnd::ParticleSys uses for an unset frame. The ring advance at 0x00476fe0 compares
    // the requested slice against it and writes the same sentinel into the mUnknown88 entry of the
    // slice it retires.
    int mUnknown7c;
    // +0x80 Starts at 0. A float the ring advance accumulates mUnknown9c divided by mUnknowna0 into
    // once per step, and reloads with the requested slice scaled by mUnknown9c on a fresh slice.
    float mUnknown80;
    // +0x84 Starts at 0. The step counter the ring advance reloads from mUnknowna0 and counts down.
    int mUnknown84;
    // +0x88 Starts at 0. A word array of mUnknown40 entries, one slice identifier per slice, which
    // the scroll at 0x00476f48 reads and the ring advance writes.
    int *mUnknown88;
    unsigned char mUnknown8c[0x0c]; // +0x8c Unrecovered.
    int mUnknown98;                 // +0x98 Starts at 0.
    float mUnknown9c;               // +0x9c Starts at 0. The numerator of the per-step increment.
    int mUnknowna0;                 // +0xa0 Unrecovered. The divisor of the per-step increment.
    // +0xa4 The mesh grid, addressed as `[slice % mUnknown40][ring % mUnknown3c]` by the lookup at
    // 0x00477388, which then returns the first element of the inner vector. The 0xc-byte stride and
    // the clear at 0x0046acf0 destroying each slot through the vector destructor at 0x00476a80 are
    // what establish the element type.
    std::vector<std::vector<Mesh *> > mUnknowna4;
    // +0xb0 The per-slice mesh lists, addressed as `[slice % mUnknown40]` by the lookup at
    // 0x004773e8 on the same evidence as mUnknowna4.
    std::vector<std::vector<Mesh *> > mUnknownb0;
    // +0xbc Starts at 0. The first slice of the window the scroll at 0x00476f48 walks, which runs
    // mUnknown40 slices from here.
    int mUnknownbc;
    // +0xc0 One transform per ring. The tangent interpolation at 0x00477538 reads the translation
    // row of entry `i` and entry `(i + 1) % mUnknown3c` and blends the two on the vector unit.
    std::vector<Transform> mUnknownc0;
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
