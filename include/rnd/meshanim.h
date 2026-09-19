#pragma once

#include <list>
#include <vector>

#include "math/color.h"
#include "math/vector2.h"
#include "math/vector3.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"

class FailSink;
namespace Rnd {
class Mesh;
class Stream;
} // namespace Rnd

namespace Rnd {

/**
 * Per-vertex animation of one mesh.
 *
 * `Q23Rnd8MeshAnim` in the RTTI descriptor at `0x008ef4a0`, with `Rnd::Animatable` as its one
 * public base at offset 0. The Animatable subobject is 0x18 bytes and the members below start
 * after it. The shared Rnd::Object subobject sits at `+0x2c`, which the Object sub-vtable at
 * `0x0081e2c0` pins from outside by recording a `this` adjustment of -0x2c in every entry, and
 * the creator at `0x00493a00` allocates 0x48 bytes for the whole object. The primary vtable is at
 * `0x0081e308` and has four entries, GetTypeInfo, EndFrame(), the inherited
 * Rnd::Animatable::StartAnim(), and SetFrameSelf(), followed by an all-zero terminator.
 *
 * Three channels animate the mesh, one for the vertex positions, one for the first texture
 * coordinate, and one for the vertex colours. The text dump titles them "vertPointsKeys:",
 * "vertTexsKeys:", and "vertColorsKeys:", which is where the member names come from. Each channel
 * is a `std::list` of keyframes, and one keyframe stores a whole vector of per-vertex values plus
 * the frame it applies at. A mesh anim whose keys belong to another mesh anim reads that object's
 * channels instead of its own, the same sharing arrangement Rnd::Mesh uses for its geometry.
 *
 * The two object references are titled "light:" and " keysOwner:" by the dump. The first label is
 * wrong in the shipped program: Replace() at `0x00486ae8` and Copy() at `0x00487288` both narrow
 * the incoming object with the Rnd::Mesh type descriptor at `0x00492528` before storing it, and
 * SetFrameSelf() reads `mVertsOwner` at `+0x130` and the vertex vector at `+0xe8` through it, so
 * the field is the animated mesh. The label is a copy-paste remnant from a sibling class.
 */
class MeshAnim : public Animatable {
public:
    /** Serial version this build writes, and the highest version it loads. */
    enum { kSerialVersion = 0 };

    /**
     * Bit of the copy flags that shares the source's keyframe channels rather than copying them.
     *
     * Recovered from Copy() at `0x004872fc`. The bit continues the series Rnd::Mesh reads,
     * kCopyShareVerts through kCopyShareTransforms.
     */
    enum { kCopyShareKeys = 0x40 };

    /**
     * One keyframe of the vertex position channel.
     *
     * The record is 0x10 bytes, a `std::vector` of three pointers followed by the frame. Its
     * member names are inferred from the dump, which writes "(frame:" for the second member and
     * " value:" ahead of the vector.
     */
    struct PointsKey {
        std::vector<Vector3> mValues; /*!< One position per mesh vertex. +0x00 */
        float mFrame;                 /*!< Frame the values apply at. +0x0c */
    };

    /** One keyframe of the first texture coordinate channel. */
    struct TexsKey {
        std::vector<Vector2> mValues; /*!< One texture coordinate per mesh vertex. +0x00 */
        float mFrame;                 /*!< Frame the values apply at. +0x0c */
    };

    /** One keyframe of the vertex colour channel. */
    struct ColorsKey {
        std::vector<Color> mValues; /*!< One colour per mesh vertex. +0x00 */
        float mFrame;               /*!< Frame the values apply at. +0x0c */
    };

    /**
     * Construct an animation with no keyframes that owns its own channels.
     *
     * @param name The object name, passed to the Rnd::Object constructor.
     * @ghidraAddress 0x00493520
     */
    MeshAnim(const HxStr &name);

    /** @ghidraAddress 0x00493240 */
    virtual ~MeshAnim();

    /**
     * Report the frame of the last keyframe across the three channels.
     *
     * Rnd::Animatable vtable slot 1. An empty channel reports zero, and the channels read belong
     * to mKeysOwner rather than to this object.
     *
     * @return The largest keyframe timestamp, never below zero.
     * @ghidraAddress 0x004873b0
     */
    virtual float EndFrame();

    /**
     * Write the animation to the engine text sink.
     *
     * Emits the Rnd::Object and Rnd::Animatable dumps, then the "[MeshAnim]" block. The block is
     * suppressed while the dump level of the sink is zero or negative.
     *
     * @param sink The text sink.
     * @ghidraAddress 0x00486c98
     */
    virtual void DumpText(FailSink &sink);

    /**
     * Serialise the animation.
     *
     * Writes kSerialVersion, the Rnd::Animatable subobject, the mesh reference as a name, the
     * three channels, and finally the keys owner as a name.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x00486e50
     */
    virtual void Save(Stream &stream);

    /**
     * Replace one object reference with another.
     *
     * Forwards to Rnd::Animatable, then swaps the mesh and the keys owner whose current value is
     * the old object. Losing the keys owner makes this object copy the channels it was sharing and
     * become its own owner rather than lose them.
     *
     * @param pFrom The object being replaced.
     * @param pTo The object to point at, which may be null.
     * @ghidraAddress 0x00486ac0
     */
    virtual void Replace(Object *pFrom, Object *pTo);

    /**
     * Return the registered class name, "MeshAnim".
     *
     * @return The class name.
     * @ghidraAddress 0x00493510
     */
    virtual const HxStr &ClassName() const;

    /**
     * Copy another animation over this one.
     *
     * kCopyShareKeys shares the source's channels instead of copying them, and a source that is
     * itself sharing is always shared from rather than copied.
     *
     * @param pSource The source object, which has to be a mesh anim for the copy to have any
     *                effect.
     * @param nFlags The copy flags.
     * @ghidraAddress 0x00487258
     */
    virtual void Copy(const Object *pSource, unsigned nFlags);

    /**
     * Load the animation.
     *
     * Reports "Can't load new MeshAnim" through the failure sink when the file version exceeds
     * kSerialVersion. Object references arrive as names and resolve through Rnd::g_manager with a
     * checked cast.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x00486fa8
     */
    virtual void Load(Stream &stream);

    /**
     * Copy one vertex slot over another in every keyframe of all three channels.
     *
     * The channels read and written belong to mKeysOwner. The welding pass at `0x00483e70` is the
     * only caller, which is why the method is public.
     *
     * A warning for whoever reconstructs that welding pass. Its seam-normal helper at `0x00483438`
     * calls `0x00569bf0`, and that routine is not Harmonix code; it is the bipartite matching of
     * Setubal's netflow package, vendored into the image, which its own diagnostics
     * `"Inconsistent matching between %d(U) and %d(V)"` and `"matching NOT maximum; augm. path:"`
     * establish, along with the package data path the image stores at `0x0083c160`. The game code
     * is the part that builds the vertex graph and reads the matching back. The matcher itself is
     * upstream and is not to be reconstructed, so it wants an external declaration rather than a
     * body.
     *
     * @param nFromVert The vertex slot to copy from.
     * @param nToVert The vertex slot to copy over.
     * @ghidraAddress 0x004867d8
     */
    void CopyVertKeys(int nFromVert, int nToVert);

protected:
    /**
     * Interpolate the three channels at a frame and write the result into the mesh.
     *
     * Rnd::Animatable vtable slot 3. Does nothing while mMesh is null. Each channel selects the
     * keyframe pair bracketing the frame, clamping to the first and the last key outside the
     * recorded range, and blends into the vertices of `mMesh->mVertsOwner`. Each channel then
     * reports its own bit to Rnd::Mesh::SyncChanged().
     *
     * @param flFrame The filtered frame to animate to.
     * @ghidraAddress 0x00487518
     */
    virtual void SetFrameSelf(float flFrame);

private:
    // Take a reference on the mesh and on the keys owner. Load() and Copy() inline it as their own
    // second half, and no standalone body survives.
    void AddObjectRefs();

    // Drop the reference on the mesh and on the keys owner. 0x00494238. The destructor calls it and
    // Load(), Copy(), and Replace() inline it.
    void RemoveObjectRefs();

    // Data members follow the recovered offset order, and the access specifiers interleave.

    // The mesh whose vertices this animation drives. The dump titles it "light:"; see the class
    // documentation for why the label is wrong.
    Mesh *mMesh; // +0x18
    std::list<PointsKey> mVertPointsKeys;
    std::list<TexsKey> mVertTexsKeys;
    std::list<ColorsKey> mVertColorsKeys;

public:
    /*!< Animation whose channels this one reads, itself for an animation that owns its keys.
         Public because the welding pass at `0x00483f88` reads it to decide whether an animation
         may be rebuilt, and the image has no accessor for it. +0x28 */
    MeshAnim *mKeysOwner;
};

/**
 * Allocate and construct a mesh animation for the registered "MeshAnim" class.
 *
 * Rnd::Manager::Init() registers this factory under g_meshAnimClassName. There is no replaceable
 * creator hook of the kind Rnd::Mesh has, because no platform subclass of this class exists.
 *
 * @param name The object name.
 * @return The new animation, as its Rnd::Object subobject.
 * @ghidraAddress 0x00493a00
 */
Object *CreateRegisteredMeshAnim(const HxStr &name);

/**
 * Registered class name of Rnd::MeshAnim, the string "MeshAnim".
 *
 * @ghidraAddress 0x006eed70
 */
extern HxStr g_meshAnimClassName;

} // namespace Rnd
