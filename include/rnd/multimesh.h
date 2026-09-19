#pragma once

#include <list>

#include "math/transform.h"
#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/drawable.h"
#include "rnd/mesh.h"
#include "rnd/stream.h"

namespace Rnd {

/**
 * Draws one mesh many times, once per recorded instance transform.
 *
 * `Q23Rnd9MultiMesh` in the RTTI descriptor at `0x008ef150`, with `Rnd::Drawable` as its one
 * public base at offset 0. The Drawable subobject is 0x14 bytes and the shared Rnd::Object
 * subobject sits at `+0x1c`, which the Object sub-vtable at `0x00823e70` pins from outside by
 * recording a `this` adjustment of -0x1c in every entry. The creator allocates 0x38 bytes, which
 * the 0x1c of Rnd::Object accounts for exactly. The Object sub-vtable has eight entries and the
 * Drawable sub-vtable at `0x00823eb8` has four, each followed by an all-zero terminator.
 *
 * An instance is a bare Rnd::Transform of 0x40 bytes. An earlier reading of this class put two
 * undetermined words in front of the transform and made the record 0x50 bytes. Both were wrong,
 * and the 0x50 was the size of the list node rather than of the record. The constructor at
 * `0x004e8a98` gives the node pool an element size of 0x40 and then allocates a 0x50-byte node,
 * which places the payload at node + 0x10 rather than at the node + 0x08 a four-byte payload
 * would use. The reader at `0x004eb6a0` confirms the layout from the other side by building a
 * 0x40-byte prototype whose four padding words it sets to 1.0 and then reading three floats into
 * each of the four rows.
 *
 * The hardware submission path belongs to Rnd::PsMultiMesh, and DrawSelf() below is the portable
 * implementation it replaces.
 */
class MultiMesh : public Drawable {
public:
    /** Serial version this build writes, and the highest version it loads. */
    enum { kSerialVersion = 0 };

    /**
     * Construct an empty multi-mesh.
     *
     * @param name The object name, passed to the Rnd::Object constructor.
     * @ghidraAddress 0x004e8830
     */
    MultiMesh(const HxStr &name);

    /** @ghidraAddress 0x004e85c0 */
    virtual ~MultiMesh();

    /**
     * Write the multi-mesh to the engine text sink.
     *
     * Emits the Rnd::Object and Rnd::Drawable dumps, then the "[MultiMesh]" block. The block is
     * suppressed while the dump level of the sink is zero or negative.
     *
     * @param sink The text sink.
     * @ghidraAddress 0x004e81a0
     */
    virtual void DumpText(FailSink &sink);

    /**
     * Serialise the multi-mesh.
     *
     * Writes kSerialVersion, the Rnd::Drawable subobject, the mesh reference as a name, and the
     * transform list.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x004ebd10
     */
    virtual void Save(Stream &stream);

    /**
     * Replace one object reference with another.
     *
     * Forwards to Rnd::Drawable, then swaps the instanced mesh when its current value is the old
     * object.
     *
     * @param pFrom The object being replaced.
     * @param pTo The object to point at, which may be null.
     * @ghidraAddress 0x004ebe40
     */
    virtual void Replace(Object *pFrom, Object *pTo);

    /**
     * Return the registered class name, "MultiMesh".
     *
     * @return The class name.
     * @ghidraAddress 0x004eba80
     */
    virtual const HxStr &ClassName() const;

    /**
     * Copy another multi-mesh over this one.
     *
     * Takes the source's mesh reference and its whole transform list. No copy flag alters either,
     * so an instanced mesh is always shared rather than duplicated.
     *
     * @param pSource The source object, which has to be a multi-mesh for the copy to have any
     *                effect.
     * @param nFlags The copy flags, forwarded to Rnd::Drawable.
     * @ghidraAddress 0x004ebc60
     */
    virtual void Copy(const Object *pSource, unsigned nFlags);

    /**
     * Load the multi-mesh.
     *
     * Reports "Can't load new MultiMesh" through the failure sink when the file version exceeds
     * kSerialVersion. The mesh reference arrives as a name and resolves through Rnd::g_manager
     * with a checked cast.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x004e8288
     */
    virtual void Load(Stream &stream);

    /**
     * Take a reference on the instanced mesh.
     *
     * Does nothing when mMesh is null.
     *
     * @ghidraAddress 0x004ebde0
     */
    void AcquireMeshRef();

    /**
     * Release the reference on the instanced mesh.
     *
     * The mirror of AcquireMeshRef().
     *
     * @ghidraAddress 0x004ebe10
     */
    void ReleaseMeshRef();

protected:
    /**
     * Draw the mesh once per instance.
     *
     * Rnd::Drawable vtable slot 3. Borrows the instanced mesh: the level of detail threshold and
     * both transforms are saved, each instance transform is installed as the mesh's local
     * transform in turn, and the mesh draws itself at each one. The saved state is then restored.
     *
     * @return Non-zero, always, so the children are drawn as well.
     * @ghidraAddress 0x004e83d0
     */
    virtual int DrawSelf();

    // Both members are protected because Rnd::PsMultiMesh reads the mesh and walks the transform
    // list to submit its GIF packets. The order below is the recovered offset order.
    Mesh *mMesh;                      // +0x14
    std::list<Transform> mTransforms; // +0x18
};

/**
 * Allocate and construct a multi-mesh.
 *
 * This is the creator the multi-mesh class registers with Rnd::Manager, invoked through the hook
 * below.
 *
 * @param name The object name.
 * @return The new multi-mesh.
 * @ghidraAddress 0x004ebbe8
 */
MultiMesh *NewMultiMesh(const HxStr &name);

/**
 * Build a multi-mesh for the registered "MultiMesh" class.
 *
 * Calls through g_pfnNewMultiMesh and narrows the result to its Rnd::Object subobject. The image
 * has a second byte-identical copy of this routine at `0x004eb998`, which is the same inline
 * function emitted in the Rnd::PsMultiMesh translation unit as well.
 *
 * @param name The object name.
 * @return The new multi-mesh, as its Rnd::Object subobject.
 * @ghidraAddress 0x004ebb58
 */
Object *CreateRegisteredMultiMesh(const HxStr &name);

/**
 * Point g_pfnNewMultiMesh at NewMultiMesh() and register the "MultiMesh" class with Rnd::Manager.
 *
 * No call site survives in the shipped program, because Rnd::Manager::Init() performs both steps
 * itself. The routine is dead code in the original rather than an unfinished analysis, and the
 * image has a second byte-identical copy at `0x005b5be8` from the Rnd::PsMultiMesh translation
 * unit. Ghidra attributes the second copy to Rnd::PsMultiMesh, which is wrong. Both copies install
 * the portable creator rather than the PlayStation 2 one.
 *
 * @ghidraAddress 0x004eb958
 */
void RegisterMultiMeshClass();

/**
 * Creator the registered "MultiMesh" class builds through.
 *
 * GfxDevice::Init() overwrites the hook at `0x0049af88` with Rnd::NewPsMultiMesh, so a multi-mesh
 * loaded from a file on the PlayStation 2 is a PsMultiMesh.
 *
 * @ghidraAddress 0x00704070
 */
extern MultiMesh *(*g_pfnNewMultiMesh)(const HxStr &name);

/**
 * Registered class name of Rnd::MultiMesh, the string "MultiMesh".
 *
 * @ghidraAddress 0x00704068
 */
extern HxStr g_multiMeshClassName;

/**
 * Serial version of the multi-mesh record currently being read.
 *
 * Load() reads the version out of the file into this global through the pointer the stream
 * receives, which is why no store to it appears in the routine. The arrangement matches
 * Rnd::g_nRndMeshLoadVersion.
 *
 * @ghidraAddress 0x00895038
 */
extern int g_nRndMultiMeshLoadVersion;

} // namespace Rnd
