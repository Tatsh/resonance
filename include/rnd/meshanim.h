#pragma once

#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/mesh.h"
#include "rnd/stream.h"

namespace Rnd {

/**
 * Per-vertex animation of one mesh.
 *
 * `Q23Rnd8MeshAnim` in the RTTI descriptor at `0x008ef4a0`, with `Rnd::Animatable` as its one
 * public base at offset 0. The Animatable subobject is 0xc bytes, so the members below start after
 * it.
 *
 * Three channels animate the mesh, one for the vertex positions, one for the texture coordinates,
 * and one for the vertex colours. The text dump titles them "vertPointsKeys:", "vertTexsKeys:",
 * and "vertColorsKeys:", which is where the member names come from, and it titles the two
 * references "light:" and " keysOwner:". A mesh anim whose keys belong to another mesh anim reads
 * that object's channels instead of its own.
 *
 * Recovery is partial. The members between the base subobject and `+0x18` are undetermined, and
 * the keyframe record of each channel is not reconstructed. The routines recovered so far are the
 * frame apply at `0x00486ac0`, the key interpolation at `0x004867d8`, the text dump at
 * `0x00486c98`, the load at `0x00486fa8`, the mesh reference setter at `0x00487258`, the welding
 * pass at `0x00483e70` with its helpers at `0x004832d0` and `0x00483438`, the three channel dumps
 * between `0x00490840` and `0x00490e78`, the three channel loads between `0x00491678` and
 * `0x00491f80`, and the six channel assignment helpers between `0x0048f6c0` and `0x004920c8`.
 */
class MeshAnim : public Animatable {
public:
    /** @ghidraAddress 0x00486c98 */
    virtual void DumpText(FailSink &sink);

    /** @ghidraAddress 0x00486fa8 */
    virtual void Load(Stream &stream);

    /**
     * Apply the animation at a frame.
     *
     * @param flFrame The frame to apply.
     * @ghidraAddress 0x00486ac0
     */
    void SetFrame(float flFrame);

    /**
     * Point the animation at the mesh it drives.
     *
     * @param pMesh The mesh.
     * @ghidraAddress 0x00487258
     */
    void SetMeshRef(Mesh *pMesh);

    /**
     * Merge coincident vertices of the driven mesh and rebuild the normals across the seams.
     *
     * @ghidraAddress 0x00483e70
     */
    void WeldMesh();

private:
    // No class derives from Rnd::MeshAnim and no access from outside it is recovered, so every
    // member is private. The order below is the recovered offset order.
    int mUnknown0c;       // +0x0c
    int mUnknown10;       // +0x10
    int mUnknown14;       // +0x14
    int mLight;           // +0x18 The dump titles the field "light:", so it references a light.
    int mVertPointsKeys;  // +0x1c Sentinel of the vertex position channel.
    int mVertTexsKeys;    // +0x20 Sentinel of the texture coordinate channel.
    int mVertColorsKeys;  // +0x24 Sentinel of the vertex colour channel.
    MeshAnim *mKeysOwner; // +0x28
};

/**
 * Registered class name of Rnd::MeshAnim, the string "MeshAnim".
 *
 * @ghidraAddress 0x006eed70
 */
extern HxStr g_meshAnimClassName;

} // namespace Rnd
