#pragma once

#include <vector>

#include "math/color.h"
#include "math/vector2.h"
#include "math/vector3.h"
#include "os/hxstr.h"
#include "rnd/collideable.h"
#include "rnd/drawable.h"
#include "rnd/transformable.h"

class FailSink;
namespace Rnd {
class Mat;
class Mesh;
class Object;
class Stream;
struct MeshVert;
} // namespace Rnd

namespace Rnd {

/**
 * Screen-aligned ribbon drawn through a list of world-space points.
 *
 * `Q23Rnd6String` in the RTTI descriptor at `0x008efd50`, whose name string is at `0x00821700` and
 * whose three base entries at `0x00821710` record `Rnd::Drawable` at offset 0,
 * `Rnd::Transformable` at offset 32, and `Rnd::Collideable` at offset 208, each non-virtual and
 * public. All three derive virtually from `Rnd::Object`. One shared Object subobject therefore sits
 * at `+0x110`. The class is 0x130 bytes, a size the factory at `0x004bedc8` pins by allocating
 * exactly that much. The members end at `+0x108`, and the 16-byte alignment the transform rows
 * impose rounds the virtual base up to `+0x110`.
 *
 * This class is unrelated to HxStr, the engine's text string type. The title comes from the RTTI
 * descriptor, and the object is a length of ribbon geometry the way a guitar string is one.
 *
 * Four vtables belong to the class, each ending in an all-zero terminator entry. The four-entry
 * table at `0x00821618` is addressed by the `Rnd::Drawable` vptr at `+0x10` and overrides
 * SetHighlight() and DrawSelf(). The four-entry table at `0x008215f8` is addressed by the
 * `Rnd::Transformable` vptr at `+0xc8` and overrides nothing. The three-entry table at
 * `0x008215d8` is addressed by the `Rnd::Collideable` vptr at `+0xd8` and overrides Collide(). The
 * nine-entry table at `0x00821640` is addressed by the Object subobject vptr and stores the eight
 * `Rnd::Object` slots with a `-0x110` adjustment on each.
 *
 * The geometry is not drawn directly. Every string owns one Rnd::Mesh that CreateMesh() builds
 * under the name `[<object name>_mesh]`, and that mesh is what the renderer submits. Each point
 * governs two mesh vertices, or four where a cap meets it, and DrawSelf() rewrites the vertex
 * positions every frame from the projected screen positions.
 *
 * The member titles come from the text DumpText() writes, "[String]", "points:", "width:",
 * " foldAngle:", " hasCaps:", "linePairs:", and "mesh:", and from the per-point labels "\n\tv:"
 * and "\n\tc:". Load() rejects a version of 3 or above with "Can't load new String".
 *
 * Ghidra shipped the name `RndBlur__*` on the routines of this class, and one worklist also
 * attributed the path predicate at `0x004e7cd8` to it. Neither attribution is supported by the
 * vtables or by the type-info accessor at `0x004bed30`.
 */
class String : public Drawable, public Transformable, public Collideable {
public:
    /**
     * One point of the ribbon.
     *
     * The first two members serialise and dump, under the labels "v" and "c". Everything after
     * them is frame scratch that DrawSelf() rewrites. The default constructor accordingly writes
     * none of it.
     *
     * The type is public because the three vector helpers of the Rnd::String translation unit
     * receive the vector by address rather than through the class, at `0x004be168`, `0x004be408`,
     * and `0x004be5a8`. A friend declaration for those three fits the image equally well, and
     * public asserts the weaker of the two.
     */
    struct Point {
        /**
         * Construct a point at the origin, coloured white.
         *
         * The constructor has no address of its own. Its body appears inlined at each of its
         * three call sites, in SetNumPoints() and twice inside the vector reader at `0x004be5a8`.
         */
        Point();

        Vector3 mPos;    /*!< Position in the object's own space. +0x00 */
        Color mColor;    /*!< Colour of both mesh vertices this point governs. +0x10 */
        Vector3 mCamPos; /*!< Camera-space position, rewritten by DrawSelf(). +0x20 */
        Vector2 mScreen; /*!< Projected position, rewritten by DrawSelf(). +0x30 */
        Vector2 mDir;    /*!< Unit screen direction towards the next point. +0x38 */
        Vector2 mNormal; /*!< Perpendicular of mDir, (-mDir.y, mDir.x). +0x40 */
        int mClipped;    /*!< Set while the point sits behind the near plane. +0x48 */
        int mUnknown4c;  /*!< Purpose not recovered. +0x4c */
    };

    /**
     * Construct an empty ribbon and its mesh.
     *
     * The width starts at 1, the fold angle at 1.5707963, the cap flag set, and the line-pair flag
     * clear. The point vector starts empty and the material starts null. The constructor never
     * writes the mesh pointer; the CreateMesh() call it ends with is what fills it.
     *
     * @param name The registry key for this object.
     * @ghidraAddress 0x004ba898
     */
    explicit String(const HxStr &name);

    /**
     * Release the owned mesh and the point vector.
     *
     * @ghidraAddress 0x004beee8
     */
    virtual ~String();

    /**
     * Resize the ribbon to nCount points and rebuild the mesh around it.
     *
     * The new points arrive default-constructed, at the origin and white. The mesh vertex vector
     * is resized to two vertices per point, plus four more when the cap flag is set, and the index
     * list is rewritten as two triangles for every pair of rungs. Every vertex then receives the
     * colour of its point and the texture coordinate of its position along the ribbon, 0 across a
     * start cap, 0.5 across the body, and 1 across an end cap.
     *
     * A nCount of zero or below resizes the point vector and stops. The mesh then retains whatever
     * geometry it already had.
     *
     * @param nCount The number of points.
     * @ghidraAddress 0x004b9b30
     */
    void SetNumPoints(int nCount);

    /**
     * Report how many points the ribbon runs through.
     *
     * @return The point count.
     * @ghidraAddress 0x004bf3c0
     */
    int GetNumPoints() const;

    /**
     * Move one point.
     *
     * Nothing is rebuilt. DrawSelf() transforms every position each frame, and a moved point
     * therefore arrives in the mesh at the next draw. The copy moves one quadword. The argument is
     * accordingly a whole padded vector rather than three floats.
     *
     * @param nIndex The point to move.
     * @param pos The new position.
     * @ghidraAddress 0x004bf738
     */
    void SetPointPos(int nIndex, const Vector3 &pos);

    /**
     * Report the address of the position of one point.
     *
     * The routine returns the address of the point itself, and the position is its first member. A
     * reading that hands back the whole point therefore fits the image equally well. No call site
     * remains in the shipped program to separate the two.
     *
     * @param nIndex The point to address.
     * @return The position.
     * @ghidraAddress 0x004bf400
     */
    Vector3 *GetPointPos(int nIndex);

    /**
     * Set the colour of one point and of the mesh vertices it governs.
     *
     * Two vertices receive the colour, or four where the point is a cap. Ends by reporting
     * Rnd::Mesh::kSyncColors to the mesh.
     *
     * @param nIndex The point to colour.
     * @param color The new colour.
     * @ghidraAddress 0x004bf758
     */
    void SetPointColor(int nIndex, const Color &color);

    /**
     * Report the address of the colour of one point.
     *
     * No call site remains in the shipped program.
     *
     * @param nIndex The point to read.
     * @return The colour.
     * @ghidraAddress 0x004bf418
     */
    Color *GetPointColor(int nIndex);

    /**
     * Set the material the ribbon draws with.
     *
     * The material is handed straight to the owned mesh and the stored material is not updated. A
     * later CreateMesh() therefore restores the material a load or a copy supplied.
     *
     * @param pMat The material, or null for none.
     * @ghidraAddress 0x004bf668
     */
    void SetMat(Mat *pMat);

    /**
     * Report the material the ribbon draws with.
     *
     * The value comes from the owned mesh rather than from the stored material.
     *
     * @return The material, or null when none is set.
     * @ghidraAddress 0x004bf500
     */
    Mat *GetMat() const;

    /**
     * Report the width of the ribbon in screen units.
     *
     * @return The width.
     * @ghidraAddress 0x004bf3e0
     */
    float GetWidth() const;

    /**
     * Set the width of the ribbon in screen units.
     *
     * Only the stored width changes. The ribbon is not rebuilt.
     *
     * @param flWidth The width.
     * @ghidraAddress 0x004bee78
     */
    void SetWidth(float flWidth) {
        mWidth = flWidth;
    }

    /**
     * Set the bend the ribbon folds at and cache its sine.
     *
     * @param flAngle The angle in radians.
     * @ghidraAddress 0x004bf708
     */
    void SetFoldAngle(float flAngle);

    /**
     * Report the bend the ribbon folds at.
     *
     * @return The angle in radians.
     * @ghidraAddress 0x004bf3f8
     */
    float GetFoldAngle() const;

    /**
     * Set whether the ribbon ends in a pair of tapering caps.
     *
     * The flag changes how many vertices each point governs. The mesh is accordingly rebuilt at
     * the current point count.
     *
     * @param nHasCaps Non-zero for capped ends.
     * @ghidraAddress 0x004bf688
     */
    void SetHasCaps(int nHasCaps);

    /**
     * Report the cap flag.
     *
     * @return Non-zero when the ribbon ends in caps.
     * @ghidraAddress 0x004bf3e8
     */
    int GetHasCaps() const;

    /**
     * Set whether consecutive points form independent segments rather than one continuous ribbon.
     *
     * The mesh is rebuilt at the current point count, on the same grounds as SetHasCaps().
     *
     * @param nLinePairs Non-zero to draw each pair of points as its own segment.
     * @ghidraAddress 0x004bf6c8
     */
    void SetLinePairs(int nLinePairs);

    /**
     * Report the line-pair flag.
     *
     * @return Non-zero when each pair of points is its own segment.
     * @ghidraAddress 0x004bf3f0
     */
    int GetLinePairs() const;

    /**
     * Pass the highlight treatment to the owned mesh.
     *
     * Rnd::Drawable vtable slot 2. Rnd::Drawable::mHighlight is not written. GetHighlight() on a
     * string therefore never reports the value that was set. The behaviour matches the binary.
     *
     * @param nHighlight Non-zero to highlight.
     * @ghidraAddress 0x004bf638
     */
    virtual void SetHighlight(int nHighlight);

    /**
     * Test a ray against the owned mesh and report the intersections as this object.
     *
     * Rnd::Collideable vtable slot 1. Tests nothing while Rnd::Drawable::mShowing is clear. Every
     * intersection the mesh appends is rewritten to address this string. A caller therefore never
     * receives the owned mesh. Ends by chaining to Rnd::Collideable::Collide() for the children.
     *
     * @param ray The segment to test along.
     * @param sink The collector to append intersections to.
     * @ghidraAddress 0x004bf570
     */
    virtual void Collide(const Ray &ray, HitSink &sink);

    /**
     * Write a description of this ribbon to sink.
     *
     * The four base descriptions come first, and everything below is produced only at a positive
     * dump level. The mesh name follows at dump level 2 or above.
     *
     * @param sink The diagnostic sink to write to.
     * @ghidraAddress 0x004ba038
     */
    virtual void DumpText(FailSink &sink);

    /**
     * Write this ribbon's serialised form to stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x004ba258
     */
    virtual void Save(Stream &stream);

    /**
     * Repoint the base subobjects when an object they address is replaced.
     *
     * Rnd::Object vtable slot 4. The routine touches no field of this class. The mesh pointer is
     * not repointed, because the string owns its mesh rather than referring to one.
     *
     * @param pFrom The object going away.
     * @param pTo The object to store instead, or null.
     * @ghidraAddress 0x004bf510
     */
    virtual void Replace(Object *pFrom, Object *pTo);

    /**
     * Report the class key a `.rnd` file writes for a ribbon.
     *
     * The returned string is g_stringClassName. The static initialiser at `0x004beb40` fills it
     * with "String".
     *
     * @return The class key.
     * @ghidraAddress 0x004bf430
     */
    virtual const HxStr &ClassName() const;

    /**
     * Copy the state of pSource into this ribbon.
     *
     * The owned mesh is discarded and rebuilt rather than copied, and the material is taken from
     * the material of the source's mesh rather than from the source's stored material.
     *
     * @param pSource The ribbon to copy from.
     * @param nFlags The set of fields to copy; see kCopyChildLists.
     * @ghidraAddress 0x004bf858
     */
    virtual void Copy(const Object *pSource, unsigned nFlags);

    /**
     * Read this ribbon's serialised form from stream.
     *
     * A version of 3 or above is rejected with "Can't load new String". The fold angle and the cap
     * flag are present from version 1 and the line-pair flag from version 2. The material is
     * resolved by name through Rnd::g_manager.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x004ba678
     */
    virtual void Load(Stream &stream);

    /**
     * Build a ribbon the class registry vends.
     *
     * @param name The registry key for the new ribbon.
     * @return The new ribbon.
     * @ghidraAddress 0x004bedc8
     */
    static String *NewString(const HxStr &name);

    /**
     * Register the class key with Rnd::g_manager.
     *
     * Unlike Rnd::Blur, the class installs no creator hook of its own. No call site remains in the
     * shipped program, because `Rnd::Manager::Init` performs the same registration inline at
     * `0x00519eac`.
     *
     * @ghidraAddress 0x004bed98
     */
    static void Init();

protected:
    /**
     * Project every point and redraw the owned mesh through it.
     *
     * Rnd::Drawable vtable slot 3. Draws nothing without a current camera and nothing with fewer
     * than two points. Each point is transformed into camera space through the inverse camera
     * transform composed with Rnd::Transformable::mWorldXfm, and flagged when it sits at or in
     * front of the near plane plus 0.01. A flagged point with an unflagged neighbour is pushed
     * onto the near plane along the segment that joins them and then unflagged, and a point whose
     * neighbours are both flagged is dropped. The survivors are projected by dividing x and z by
     * the absolute value of the depth component y. EmitRibbonVerts() then fills the mesh. The mesh
     * receives Rnd::Mesh::kSyncPoints, the camera transform, and a draw.
     *
     * The body is not reconstructed. It reads the current camera through a global this header does
     * not own, and it reads two camera fields that `cam.h` does not declare.
     *
     * @return Non-zero. The children are drawn as well.
     * @ghidraAddress 0x004b95f8
     */
    virtual int DrawSelf();

private:
    /**
     * Which mesh vertices one point governs.
     *
     * The title is inferred. Only ResolvePointVertexSlot() fills the record, and only
     * SetNumPoints(), SetPointColor(), and DrawSelf() read it.
     */
    struct VertexSlot {
        int mMode;        // +0x00 One of the kVertexSlot values below.
        MeshVert *mpVert; // +0x04 First of the two or four vertices.
    };

    // A point that is neither end of a run governs two vertices, and a cap point governs four.
    enum { kVertexSlotBody = 0, kVertexSlotStartCap = 1, kVertexSlotEndCap = 2 };

    // 0x004b9a68
    // Reports the mode of point nIndex and the first mesh vertex it governs.
    void ResolvePointVertexSlot(unsigned nIndex, VertexSlot &slot);

    // 0x004ba3d0
    // Builds the owned mesh, applies the stored material and the depth state to it,
    // caches the sine of the fold angle, and sizes the geometry to the current point count. The
    // constructor, Load(), and Copy() are the callers.
    void CreateMesh();

    // 0x004bf810
    // Releases the owned mesh and clears the pointer. The destructor is the only
    // out-of-line caller, and Load() and Copy() inline the same body.
    void DeleteMesh();

    // 0x004b9008
    // Builds the screen direction and the perpendicular of every point in the closed
    // range, widens each by mWidth, and folds the ribbon wherever the turn between two segments
    // passes mFoldSin. DrawSelf() is the only caller, and the body is not reconstructed for the
    // same reason DrawSelf() is not.
    void EmitRibbonVerts(Point *pFirst, Point *pLast);

    // Declared in recovered offset order. Every member is private, because the image supplies an
    // accessor for each field anything outside the class reads.

    // Material a load or a copy supplied. SetMat() bypasses it and writes straight to the mesh,
    // so it records only what CreateMesh() is to apply.
    Mat *mpMat;                 // +0xe0
    float mWidth;               // +0xe4
    std::vector<Point> mPoints; // +0xe8
    // The mesh this string owns and submits. CreateMesh() builds it and DeleteMesh() releases it.
    Mesh *mpMesh;     // +0xf4
    int mHasCaps;     // +0xf8
    int mLinePairs;   // +0xfc
    float mFoldAngle; // +0x100
    // Sine of mFoldAngle, refreshed by SetFoldAngle() and by CreateMesh().
    float mFoldSin; // +0x104
    // +0x108 through +0x10f is alignment padding ahead of the Rnd::Object virtual base at +0x110.
};

/**
 * Class key a `.rnd` file writes for a ribbon.
 *
 * @ghidraAddress 0x006fc348
 */
extern HxStr g_stringClassName;

} // namespace Rnd
