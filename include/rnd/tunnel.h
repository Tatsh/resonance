#pragma once

#include <list>
#include <stddef.h>
#include <vector>

#include "math/transform.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/collideable.h"
#include "rnd/drawable.h"
#include "rnd/manager.h"
#include "rnd/raytest.h"
#include "rnd/tunnelevent.h"
#include "rnd/tunnelmeshchain.h"
#include "rnd/tunnelseeker.h"

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
 * have supplied one. Members are titled by the role their routines give them, or by their offset
 * where no role is recovered.
 *
 * Load() accepts a bounded version range rather than an upper bound alone, which is the only class
 * in the renderer that does. It reports "Can't load new Tunnel" above the range and "Can't load
 * old Tunnel" below it.
 *
 * The geometry is a ring of mRingCount lanes, each a three-segment trough (LaneProfile), swept
 * along the path mPath one slice of mSliceFrames frames at a time. Each slice owns one chain of
 * lane meshes, "[<name>_lat<slice>.<level>]", and each lane of each slice one chain of cell meshes,
 * "[<name>_pan<cell>.<level>]", that fill the gap to the next lane. The ring advance places one
 * column of every slice mesh per step, and DrawSelf() draws the slices of the visible window.
 * The seekers add "[<name>_seek<index>.<section>]" meshes over the lanes they highlight.
 *
 * The record the vector at `+0xdc` stores is Rnd::TunnelSeeker. The routines at `0x00477830` and
 * `0x0046e830` are members of the seeker and of its strip rather than of this class, and the three
 * seek records read private members of this class directly.
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
     * Allocate a tunnel under the tag "Rnd::Tunnel".
     *
     * @param nSize The object size the compiler supplies.
     * @return The block.
     * @ghidraAddress 0x00476218
     */
    static void *operator new(size_t nSize);

    /**
     * Release a tunnel under the tag "Rnd::Tunnel".
     *
     * @param pBlock The block.
     * @ghidraAddress 0x00476238
     */
    static void operator delete(void *pBlock);

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
     * The slice index wraps through a signed remainder against mSliceCount, so a negative index
     * addresses from the end.
     *
     * @param nSlice The slice index, taken modulo mSliceCount.
     * @return The first mesh of the slice.
     * @ghidraAddress 0x004773e8
     */
    Mesh *GetRingSection(int nSlice);

    /**
     * Report the first mesh of one ring of one slice.
     *
     * Both indices wrap through a signed remainder, the ring against mRingCount and the slice
     * against mSliceCount.
     *
     * @param nRing The ring index, taken modulo mRingCount.
     * @param nSlice The slice index, taken modulo mSliceCount.
     * @return The first mesh of the ring.
     * @ghidraAddress 0x00477388
     */
    Mesh *GetRingSection(int nRing, int nSlice);

    /**
     * Blend the translation of one ring transform towards the next.
     *
     * The successor wraps through a signed remainder against mRingCount. Only the three components
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
     * Build the frame of a point on one ring at a path frame.
     *
     * Without a path the output is the identity, with the padding words of the three basis rows
     * unwritten. Otherwise the basis rows are those of the ring transform, the translation is
     * blended towards the next ring by LerpRingSectionTangent(), and the result is concatenated
     * with the path transform at flFrame.
     *
     * @param nRing The ring.
     * @param pOut The transform to write.
     * @param flFrame The path frame.
     * @param flBlend The weight of the next ring's translation.
     * @ghidraAddress 0x0046da40
     */
    void GetRingXfm(int nRing, Transform *pOut, float flFrame, float flBlend);

    /**
     * Build the frame of a point on one ring at a path frame, with the ring pushed outwards.
     *
     * A null mPath writes the identity. Otherwise the three basis rows of the ring transform are
     * copied through, the translation row is the blend of that ring's translation and its wrapped
     * successor's, each scaled by flTangentScale, and the result is concatenated with the path
     * transform at flAnimFrame. The AppTunnel helpers (the gem trails, the sabre trail, and the
     * grid markers) call it.
     *
     * @param nRing The ring.
     * @param pOut The transform to write.
     * @param flAnimFrame The path frame.
     * @param flRingBlend The weight of the next ring's translation.
     * @param flTangentScale The scale applied to both translations.
     * @ghidraAddress 0x0046db80
     */
    void ProjectSectionToCameraSpace(
        int nRing, Transform *pOut, float flAnimFrame, float flRingBlend, float flTangentScale);

    /**
     * Bring every slice of the current window up to the advanced slice.
     *
     * Walks the mSliceCount slices starting at mUnknownbc and calls AdvanceRing() for each one
     * whose mUnknown88 entry differs from its own index.
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

    /**
     * Replace the path the tunnel follows.
     *
     * Moves the reference from the previous path to the new one, calls EndFrame() on a non-null
     * path and discards the result, and fills mUnknown88 with the 99999999 sentinel so that every
     * slice is rebuilt.
     *
     * @param pPath The path, or null.
     * @ghidraAddress 0x004770d0
     */
    void SetPath(TransAnim *pPath);

    /**
     * Evaluate the path at a frame.
     *
     * Without a path the three basis rows receive the identity with their padding words unwritten,
     * and the translation row receives (0, 0, 0, 1).
     *
     * @param pOut The transform to write.
     * @param flFrame The path frame.
     * @ghidraAddress 0x004775b0
     */
    void GetPathXfm(Transform *pOut, float flFrame);

    /**
     * Set mLaneChangeFrames.
     *
     * @param flFrames The tunnel frames a seeker takes to move one ring.
     * @ghidraAddress 0x004772c8
     */
    void SetLaneChangeFrames(float flFrames);

    /**
     * Report one seeker.
     *
     * @param nIndex The seeker index.
     * @return The seeker, or null when nIndex is out of range.
     * @ghidraAddress 0x00477298
     */
    TunnelSeeker *GetSeeker(unsigned nIndex);

    /**
     * Set the number of seekers and attach each one to this tunnel.
     *
     * Every existing seeker releases its references first. New seekers are default constructed.
     *
     * @param nCount The seeker count.
     * @ghidraAddress 0x0046cfd8
     */
    void ResizeSeekers(unsigned nCount);

    /**
     * Set the shape parameters and rebuild the geometry.
     *
     * Every seeker releases its references, BuildMesh() runs, and every seeker is attached again.
     * The program lists no caller.
     *
     * @param flUnknown38 The value of mUnknown38.
     * @param nRingCount The ring count.
     * @param nSliceCount The slice count.
     * @param nLodCount The value of mLodCount.
     * @param flUnknown48 The value of mUnknown48.
     * @param flUnknown4c The value of mUnknown4c.
     * @param flUnknown50 The value of mUnknown50.
     * @param flUnknown54 The value of mUnknown54.
     * @ghidraAddress 0x00477160
     */
    void Configure(float flUnknown38,
                   int nRingCount,
                   int nSliceCount,
                   int nLodCount,
                   float flUnknown48,
                   float flUnknown4c,
                   float flUnknown50,
                   float flUnknown54);

    /**
     * Convert a frame to a slice.
     *
     * The frame is scaled by mSlicesPerFrame and rounded down. The program lists no caller.
     *
     * @param flFrame The frame.
     * @return The slice.
     * @ghidraAddress 0x00476540
     */
    int FrameToSlice(float flFrame);

    /**
     * Colour the wall between one lane and the next on one slice.
     *
     * Colours the right wall of the lane, the left wall of the following lane, and the first end
     * cap of the lane, then reports the colour change. Both indices wrap.
     *
     * @param nRing The lane.
     * @param nSlice The slice.
     * @param color The colour.
     * @ghidraAddress 0x0046d788
     */
    void SetLaneDividerColor(int nRing, int nSlice, const Color &color);

    /**
     * Colour the floor and the second end cap of every lane on every slice.
     *
     * @param color The colour.
     * @ghidraAddress 0x0046d8e8
     */
    void SetLaneFloorColor(const Color &color);

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
    // Drop every reference Update() takes (the path, each event drawable, and each seeker's
    // objects) and delete the generated meshes. The destructor, Load(), and Copy() call it.
    // 0x00468020.
    void ReleaseRefs();

    // Write the material and the first vertex colour of every chain of both grids. 0x00468a78.
    void SaveSectionMaterials(Stream &stream);

    // Read what SaveSectionMaterials() writes and apply each entry to the chain of the same index,
    // skipping entries past the end of the grid. Before revision 36 the cell count comes from the
    // current grid rather than the stream, and before revision 37 the slice count does. 0x00468da0.
    void LoadSectionMaterials(Stream &stream);

    // Rebuild every generated mesh. The material and first vertex colour of each chain are kept
    // across the rebuild. The ring transforms and lane profiles are regenerated from mUnknown38 and
    // the four lane parameters, and mSliceSteps becomes 2 to the power of one less than mLodCount.
    // 0x004699c0.
    void BuildMesh();

    // Build one chain per slice, "[<name>_lat<slice>]", whose finest level holds a block of
    // 6 * (mSliceSteps + 1) + 8 vertices per ring. The triangles are built on
    // the chain of slice 0 and shared by the others. 0x0046adc0.
    void BuildSliceMeshes();

    // Build one chain per lane of each slice, "[<name>_lat<lane>]", holding one flat grid of four
    // rows and two end caps, with the triangles built on the first chain and shared. The program
    // lists no caller, and BuildMesh() calls BuildSliceMeshes() instead. 0x0046b830.
    void BuildLaneMeshes();

    // Build one chain per cell, "[<name>_pan<cell>]", with two rows of mSliceSteps + 1 vertices.
    // The triangles are built on the chain of cell 0 and shared by the others. 0x0046c0e8.
    void BuildCellMeshes();

    // Empty the per-material section lists. 0x0046acf0.
    void ClearMaterialSectionLists();

    // Place column mUnknown84 of the slice mUnknown7c. The path is evaluated at mUnknown80, every
    // lane profile is carried through it into the slice mesh, and the edges of the neighbouring
    // cells follow. The first and last columns also close the ends of each lane block. Column zero,
    // the last one written, resynchronises the slice mesh and every cell mesh of the slice.
    // 0x0046c638.
    void SetRingSectionFrames();

    // A signed remainder moved into [0, nCount), the form every ring and slice lookup uses.
    static int WrapIndex(int nIndex, int nCount) {
        const int nRemainder = nIndex % nCount;
        return nRemainder > -1 ? nRemainder : nRemainder + nCount;
    }

    // The cross-section of one lane in the ring frame, a trough of three segments. mPoints[0] and
    // mPoints[3] are the lane edges, drawn from the ring translations towards the neighbouring
    // rings by mUnknown4c. mPoints[1] and mPoints[2] blend each edge by mUnknown50 with the ring
    // translation pulled towards the axis by mUnknown48. mNormals[i] is the unit normal of the
    // segment from mPoints[i] to mPoints[i + 1] in the plane of the ring. The record is 0x70 bytes.
    struct LaneProfile {
        Vector3 mPoints[4];
        Vector3 mNormals[3];
    };

    // No class derives from Rnd::Tunnel. The three seek records read its members as friends, and
    // Renderer reads the two public counts. The order below is the recovered offset order.
    // Every unknown title is the offset itself, because DumpText() is a stub. The initial value
    // each member receives from the constructor is part of the evidence about it. A member with no
    // recorded value is one the constructor does not write, or one it fills through a container
    // allocation.

    float mUnknown38; // +0x38 Starts at 1.0f. The ring radius BuildMesh() places the rings at.

public:
    /*!< The ring count, 3 at construction. It is the modulus of the inner index of mUnknowna4, the
         modulus of the index of mUnknownc0, and the row stride of mUnknowna4. Public because the
         Renderer constructor reads it at `0x0042c9c8` and the image has no accessor. */
    int mRingCount;
    /*!< The slice count, 0 at construction. It is the modulus of the index of mUnknownb0, the
         modulus of the outer index of mUnknowna4, and the length of the array mUnknown88
         addresses. Public on the same evidence, read at `0x0042c9d4`. */
    int mSliceCount;

private:
    // +0x44 Starts at 2. The level count of every generated chain and the length of mUnknown68.
    int mLodCount;
    float mUnknown48; // +0x48 Starts at 0.1f. The inward pull of the lane floor.
    float mUnknown4c; // +0x4c Starts at 0.1f. The gap each lane edge keeps from the ring boundary.
    float mUnknown50; // +0x50 Starts at 0.25f. The weight of the edge in the floor points.
    float mUnknown54; // +0x54 Starts at 0.01f. The cell edge blend per slice step.
    // +0x58 Starts at 0. The path the tunnel follows, which the camera space projection at
    // 0x0046db80 and GetPathXfm() evaluate through Rnd::TransAnim::EvalFrame(). A null path makes
    // both write the identity.
    TransAnim *mPath;
    int mUnknown5c; // +0x5c Starts at 0. Copy() carries it.
    int mUnknown60; // +0x60 Starts at 0. Save() and Load() carry it.
    // +0x64 Starts at 480.0f. The tunnel frames a seeker takes to move one ring, which
    // TunnelSeeker::UpdateLane() divides the elapsed frames by.
    float mLaneChangeFrames;
    // +0x68 One level of detail threshold per chain level, each written into the mMinScreen of the
    // mesh at the matching position of its chain. Copy() assigns it through
    // `std::vector<float>::operator=`, and the setter at 0x0046d180 reads an element with `lwc1`.
    std::vector<float> mUnknown68;
    int mUnknown74; // +0x74 Starts at 1. DrawSelf() tests it.
    int mUnknown78; // +0x78 Starts at 1. DrawSelf() tests it.
    // +0x7c Starts at 99999999, which is a hand-written sentinel in the same style as the
    // -9999999.0f Rnd::ParticleSys uses for an unset frame. The ring advance at 0x00476fe0 compares
    // the requested slice against it and writes the same sentinel into the mUnknown88 entry of the
    // slice it retires.
    int mUnknown7c;
    // +0x80 Starts at 0. The path frame of the column being placed. The ring advance reloads it
    // with the slice scaled by mSliceFrames and adds mSliceFrames / mSliceSteps per column.
    float mUnknown80;
    // +0x84 Starts at 0. The column being placed, which the ring advance counts down from
    // mSliceSteps.
    int mUnknown84;
    // +0x88 One slice identifier per slice, which the scroll at 0x00476f48 reads, the ring advance
    // writes, and SetPath() fills with the 99999999 sentinel.
    std::vector<int> mUnknown88;
    // +0x94 mUnknown54 times mSliceSteps, set by BuildMesh(). SetRingSectionFrames() blends the
    // end vertices of each cell towards their neighbours by it.
    float mCellEdgeBlend;
    // +0x98 Starts at 0, and BuildMesh() sets it to the reciprocal of mSliceFrames.
    float mSlicesPerFrame;
    // +0x9c Starts at 0, and BuildMesh() sets it to 1920. The frames one slice spans.
    float mSliceFrames;
    // +0xa0 The columns of a slice less one, which BuildMesh() sets to 2 to the power of one less
    // than mLodCount. The ring advance places one column per step.
    int mSliceSteps;
    // +0xa4 The mesh grid, one chain per cell, addressed as `slice * mRingCount + ring` by the
    // lookup at 0x00477388, which then returns the finest level of the chain. The 0xc-byte stride
    // and the clear at 0x0046acf0 destroying each slot through the chain destructor at 0x00476a80
    // are what establish the element type.
    std::vector<TunnelMeshChain> mUnknowna4;
    // +0xb0 One chain per slice, addressed as `[slice % mSliceCount]` by the lookup at 0x004773e8
    // on the same evidence as mUnknowna4.
    std::vector<TunnelMeshChain> mUnknownb0;
    // +0xbc Starts at 0. The first slice of the window the scroll at 0x00476f48 walks, which runs
    // mSliceCount slices from here.
    int mUnknownbc;
    // +0xc0 One transform per ring. The tangent interpolation at 0x00477538 reads the translation
    // row of entry `i` and entry `(i + 1) % mRingCount` and blends the two on the vector unit.
    std::vector<Transform> mUnknownc0;
    // +0xcc One lane profile per ring, generated by BuildMesh().
    std::vector<LaneProfile> mLaneProfiles;
    // +0xd8 Drawables scheduled by frame, kept in ascending frame order by AddEvent(). Update()
    // walks it taking a reference on every entry, and Copy() assigns it through the list assignment
    // operator at 0x004727b0.
    std::list<TunnelEvent> mEvents;
    // +0xdc Update() calls TunnelSeeker::SetTunnel() once per element, and Copy() assigns the
    // vector through the assignment operator at 0x004728e8.
    std::vector<TunnelSeeker> mSeekers;

    friend struct TunnelSeekSection;
    friend struct TunnelSeekStrip;
    friend struct TunnelSeeker;
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
 * Build a tunnel for the registered "Tunnel" class.
 *
 * The body is that of NewTunnel(), with the result narrowed to its Rnd::Object subobject.
 * Rnd::Manager::Init() registers this factory.
 *
 * @param name The object name.
 * @return The new tunnel, as its Rnd::Object subobject.
 * @ghidraAddress 0x00476468
 */
Object *CreateRegisteredTunnel(const HxStr &name);

/**
 * Registered class name of Rnd::Tunnel, the string "Tunnel".
 *
 * @ghidraAddress 0x006eab10
 */
extern HxStr g_tunnelClassName;

/**
 * Register the "Tunnel" class with Rnd::Manager.
 *
 * An inline function. The out-of-line copy has no callers, and Rnd::Manager::Init() performs the
 * same registration itself.
 *
 * @ghidraAddress 0x00476258
 */
inline void RegisterTunnelClass() {
    g_manager.RegisterClass(g_tunnelClassName, CreateRegisteredTunnel);
}

/**
 * Stream revision of the tunnel currently being read.
 *
 * Load() stores the revision here, and the element loaders of Rnd::TunnelEvent and
 * Rnd::TunnelSeeker test it.
 *
 * @ghidraAddress 0x00894d64
 */
extern int g_nTunnelLoadVersion;

} // namespace Rnd
