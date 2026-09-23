#pragma once

#include <list>
#include <vector>

#include "math/transform.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/collideable.h"
#include "rnd/drawable.h"
#include "rnd/raytest.h"
#include "rnd/tunnelevent.h"
#include "rnd/tunnelmeshchain.h"

class FailSink;
namespace Rnd {
class Mesh;
class Object;
class Stream;
class TransAnim;
} // namespace Rnd

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
 * update at `0x00467f48`, the mesh build at `0x004699c0`, the face strip build at `0x0046adc0`, the
 * VU1 upload at `0x0046c0e8`, and the section frame setter at `0x0046c638`.
 *
 * Nine addresses a worklist grouped under this class belong elsewhere, and each one is recorded
 * here so the grouping is not repeated. `0x00466528` and `0x00476598` append triangles to the face
 * vector a `Rnd::Mesh` addresses through its own mFacesOwner at `+0x134`, which is past the end of
 * this class, so both are Rnd::Mesh members. `0x00493b60` and `0x00493bd0` set mVertsOwner and
 * mFacesOwner of a `Rnd::Mesh` at those same offsets, and `0x00494048` writes a quadword into each
 * 0x40-byte element of a range the object at `+0x130` owns, so all three are Rnd::Mesh members too.
 * `0x00473538` and `0x00474990` measure their range with an arithmetic shift of six, which is a
 * `std::vector` of 0x40-byte elements rather than anything of this class. `0x00476190` is the
 * default constructor of a `std::vector`, and `0x00476a80`, `0x00476b28`, and `0x00476e48` are
 * members of Rnd::TunnelMeshChain. `0x00472488` is `std::vector<float>::operator=`, which six
 * routines of this class and one of another share, and an earlier reading had it as a drawable
 * index table rebuild of this class. `0x004698e8` resizes `std::vector<Rnd::MeshVert>` through the
 * mVertsOwner of the first mesh of a mesh list, and an earlier reading had it as a bounding box
 * resize of this class.
 *
 * The record the vector at `+0xdc` stores is a class of its own and is not recovered. The routine
 * at `0x00477830` is one of its members rather than one of this class, which Update() pins by
 * passing the vector element in $a0, the tunnel in $a1, and the element index in $a2. The fields
 * that routine touches at `+0x50`, `+0x68`, `+0x6c`, and `+0x74` are fields of the record, and an
 * earlier reading attributed all four to this class. `0x0046e830` is a second member of the same
 * record, and between them they pin a tunnel pointer at `+0x24` and `+0x6c`, an object reference at
 * `+0x20`, `+0x50`, and `+0x68`, three counters from `+0x2c`, a `std::vector` of 0x20-byte elements
 * at `+0x38`, and the frame at `+0x74`.
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
     * are blended. The whole quadword is stored, so the padding word of the output receives the
     * padding word of the successor's translation.
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

    /**
     * Replace the level of detail thresholds and apply them to every generated mesh.
     *
     * The argument is assigned over mUnknown68, then both mesh grids are walked. Each mesh takes
     * the threshold whose index matches its position within its own list, and a mesh whose
     * position passes the end of the threshold vector is skipped rather than clamped. The title is
     * inferred from Rnd::Mesh::mMinScreen, the member it writes.
     *
     * Public because the stage classes between `0x00432000` and `0x00444000` supply nine callers,
     * none of which derives from this class. A friend declaration per calling class fits the image
     * equally well.
     *
     * @param screenSizes One threshold per ring, applied in index order.
     * @ghidraAddress 0x0046d180
     */
    void ApplyMeshLodScreenSizes(const std::vector<float> &screenSizes);

    /**
     * Schedule a drawable at a frame.
     *
     * The event goes before the first one whose frame is not less than flFrame, so equal frames
     * keep the newest first. The tunnel takes a reference on the drawable.
     *
     * @param pObject The drawable, which may be null.
     * @param flFrame The frame to schedule at.
     * @param nId The identifier MoveEvent() and RemoveEvent() match on.
     * @param nUser A word stored with the event.
     * @ghidraAddress 0x0046d400
     */
    void AddEvent(Drawable *pObject, float flFrame, int nId, int nUser);

    /**
     * Move the first event with an identifier to a new frame.
     *
     * The event is unlinked and scheduled again through AddEvent(), which takes a second reference
     * on the drawable without the first being dropped, and its user word is reset to zero. The
     * program lists no caller.
     *
     * @param nId The identifier to match.
     * @param flFrame The new frame.
     * @return One when an event matched, zero otherwise.
     * @ghidraAddress 0x0046d540
     */
    int MoveEvent(int nId, float flFrame);

    /**
     * Remove the first event with an identifier and drop its reference.
     *
     * The program lists no caller.
     *
     * @param nId The identifier to match.
     * @return One when an event matched, zero otherwise.
     * @ghidraAddress 0x0046d5d8
     */
    int RemoveEvent(int nId);

    /**
     * Remove every event whose frame lies in a half open range, dropping each reference.
     *
     * @param flFrom The first frame removed.
     * @param flTo The frame the range stops before.
     * @return The number of events removed.
     * @ghidraAddress 0x0046d680
     */
    int RemoveEventsInRange(float flFrom, float flTo);

    /**
     * Call a function once for every event, in frame order.
     *
     * The program lists no caller.
     *
     * @param pfnVisit The function, given the drawable, the frame, the identifier, and pUser.
     * @param pUser Passed through to pfnVisit.
     * @ghidraAddress 0x00477310
     */
    void ForEachEvent(void (*pfnVisit)(Drawable *pObject, float flFrame, int nId, void *pUser),
                      void *pUser);

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

    // Project one ring into camera space. 0x0046db80. A null mUnknown58 writes the identity into
    // pOut and returns. Otherwise the three basis rows of mUnknownc0[nRing] are copied through,
    // the translation row is the blend of that entry and its wrapped successor each scaled by
    // flTangentScale, and the result is concatenated through XfmConcat() with what mUnknown58
    // evaluates to at flAnimFrame.
    void ProjectSectionToCameraSpace(
        int nRing, Transform *pOut, float flAnimFrame, float flRingBlend, float flTangentScale);

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
    // +0x58 Starts at 0. The transform animation the camera space projection at 0x0046db80
    // evaluates through Rnd::TransAnim::EvalFrame(), and the object Update() takes a reference on
    // before it rebuilds. A null one makes the projection write the identity.
    TransAnim *mUnknown58;
    int mUnknown5c;   // +0x5c Starts at 0.
    int mUnknown60;   // +0x60 Starts at 0.
    float mUnknown64; // +0x64 Starts at 480.0f, which is the display height.
    // +0x68 One level of detail threshold per ring, each written into the mMinScreen of the mesh at
    // the matching position of its list. Three routines pin the shape. Copy() at 0x00476814 assigns
    // it from the source tunnel through `std::vector<float>::operator=`, the setter at 0x0046d180
    // assigns it from its argument through the same operator, and the same setter reads an element
    // with `lwc1`, which is what fixes the element as a float rather than a word.
    std::vector<float> mUnknown68;
    float mUnknown74; // +0x74 Starts at 1.
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
    // +0xa4 The mesh grid, one chain per cell, addressed as `slice * mUnknown3c + ring` by the
    // lookup at 0x00477388, which then returns the finest level of the chain. The 0xc-byte stride
    // and the clear at 0x0046acf0 destroying each slot through the chain destructor at 0x00476a80
    // are what establish the element type.
    std::vector<TunnelMeshChain> mUnknowna4;
    // +0xb0 One chain per slice, addressed as `[slice % mUnknown40]` by the lookup at 0x004773e8
    // on the same evidence as mUnknowna4.
    std::vector<TunnelMeshChain> mUnknownb0;
    // +0xbc Starts at 0. The first slice of the window the scroll at 0x00476f48 walks, which runs
    // mUnknown40 slices from here.
    int mUnknownbc;
    // +0xc0 One transform per ring. The tangent interpolation at 0x00477538 reads the translation
    // row of entry `i` and entry `(i + 1) % mUnknown3c` and blends the two on the vector unit.
    std::vector<Transform> mUnknownc0;
    int mUnknowncc;                 // +0xcc Starts at 0.
    unsigned char mUnknownd0[0x08]; // +0xd0 Unrecovered.
    // +0xd8 Drawables scheduled by frame, kept in ascending frame order by AddEvent(). Update()
    // walks it taking a reference on every entry, and Copy() assigns it through the list assignment
    // operator at 0x004727b0.
    std::list<TunnelEvent> mEvents;
    // +0xdc A std::vector whose element is the 0x80-byte record described in the class note.
    // Update() walks it calling the record member at 0x00477830 once per element, and Copy()
    // assigns it through the vector assignment operator at 0x004728e8. The element class is
    // undetermined, which is why the member is a byte run rather than a container.
    unsigned char mUnknowndc[0x0c];
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
