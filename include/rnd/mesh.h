#pragma once

#include <vector>

#include "math/sphere.h"
#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/collideable.h"
#include "rnd/drawable.h"
#include "rnd/mat.h"
#include "rnd/meshedge.h"
#include "rnd/meshface.h"
#include "rnd/meshvert.h"
#include "rnd/stream.h"
#include "rnd/transformable.h"

namespace Rnd {

/**
 * Indexed triangle mesh with one material.
 *
 * `Q23Rnd4Mesh` in the RTTI descriptor at `0x008eed58`, with three public non-virtual bases:
 * `Rnd::Drawable` at `+0x00`, `Rnd::Transformable` at `+0x20`, and `Rnd::Collideable` at `+0xd0`.
 * All three derive virtually from `Rnd::Object`, so one shared Object subobject sits at `+0x150`
 * and the whole object is 0x16c bytes; the factory rounds the allocation to 0x170.
 *
 * Geometry is shared rather than copied. A mesh whose mVertsOwner is another mesh draws that
 * mesh's vertices, and its own vertex vector is released after a load or a copy. The same applies
 * to mFacesOwner for the face and edge vectors. A newly constructed mesh owns its own geometry,
 * because the constructor sets both owners to this.
 *
 * The drawing implementation belongs to the platform subclass. Rnd::PsMesh supplies DrawSelf()
 * and Sync(), and GfxDevice::Init() replaces the creator hook at `0x006eed60` with the PsMesh
 * factory, so every mesh a file loads on the PlayStation 2 is a PsMesh.
 *
 * The compiler-generated `GetTypeInfo()` is at `0x00492528`.
 */
class Mesh : public Drawable, public Transformable, public Collideable {
public:
    /** Depth buffer read and write mode, as the text dump titles the values. */
    enum ZMode {
        kZModeDisable = 0,    /*!< No depth test and no depth write. */
        kZModeZReadOnly = 1,  /*!< Depth test against Z, no write. */
        kZModeZReadWrite = 2, /*!< Depth test against Z and write. */
        kZModeWReadOnly = 3,  /*!< Depth test against W, no write. */
        kZModeWReadWrite = 4  /*!< Depth test against W and write. */
    };

    /** Depth comparison, as the text dump titles the values. */
    enum ZFunc {
        kZFuncNever = 0,
        kZFuncLess = 1,
        kZFuncEqual = 2,
        kZFuncLessEqual = 3,
        kZFuncGreater = 4,
        kZFuncNotEqual = 5,
        kZFuncGreaterEqual = 6,
        kZFuncAlways = 7
    };

    /** Every bit SyncAll() reports as changed. */
    enum { kSyncAllMask = 0x7f };

    /**
     * Bits of the copy flags Copy() tests.
     *
     * The flags belong to the Rnd::Object copy interface. Only the three bits the mesh reads are
     * recovered, and their names are inferred from the effect each one has.
     */
    enum {
        kCopyShareVerts = 0x08,     /*!< Point at the source's vertex owner rather than copying. */
        kCopyShareFaces = 0x10,     /*!< Point at the source's face owner rather than copying. */
        kCopyShareTransforms = 0x20 /*!< Point at the source's transform owners. */
    };

    /** Serial version this build writes, and the highest version it loads. */
    enum { kSerialVersion = 10 };

    /**
     * Construct an empty mesh that owns its own geometry.
     *
     * @param name The object name, passed to the Rnd::Object constructor.
     * @ghidraAddress 0x0047ff20
     */
    Mesh(const HxStr &name);

    /** @ghidraAddress 0x00492838 */
    virtual ~Mesh();

    /**
     * Write the mesh to the engine text sink.
     *
     * Emits the three base dumps, then the "[Mesh]" block. The whole block is suppressed while
     * the dump level of the sink is zero or negative.
     *
     * @param sink The text sink.
     * @ghidraAddress 0x00480d80
     */
    virtual void DumpText(FailSink &sink);

    /**
     * Serialise the mesh.
     *
     * Writes kSerialVersion, the three base subobjects, the two depth fields, each object
     * reference as a name, the bounding sphere, the level of detail fields, and finally the
     * vertex, face, and edge vectors.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x00481300
     */
    virtual void Save(Stream &stream);

    /**
     * Replace one object reference with another.
     *
     * Forwards to the three bases, then swaps the material and the five mesh references whose
     * current value is the old object. A null replacement makes the mesh adopt what it was
     * sharing rather than lose it.
     *
     * @param pFrom The object being replaced.
     * @param pTo The object to point at, which may be null.
     * @ghidraAddress 0x00482810
     */
    virtual void Replace(Object *pFrom, Object *pTo);

    /**
     * Return the registered class name, "Mesh".
     *
     * The key is the one a data file writes for the class, which the registry at
     * `Rnd::Manager::Init()` maps to the creator below.
     *
     * @return The class name.
     * @ghidraAddress 0x00492f00
     */
    virtual const HxStr &ClassName() const;

    /**
     * Copy another mesh over this one.
     *
     * Bit 3 of the flags shares the vertex vector instead of copying it, bit 4 does the same for
     * the face and edge vectors, and bit 5 does the same for the three transform references. A
     * shared vector is then released by ClearSharedGeometry().
     *
     * @param pSource The source object, which has to be a mesh for the copy to have any effect.
     * @param nFlags The copy flags.
     * @ghidraAddress 0x00482568
     */
    virtual void Copy(const Object *pSource, unsigned nFlags);

    /**
     * Load the mesh.
     *
     * Reports "Can't load new Mesh" through the failure sink when the file version exceeds
     * kSerialVersion. Object references arrive as names and resolve through Rnd::g_manager with a
     * checked cast. A name that no loaded object matches produces a null reference. Versions below
     * 10 are all still readable, and each version test is documented at its reading site.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x004817d0
     */
    virtual void Load(Stream &stream);

    /**
     * Point the mesh at a material.
     *
     * Drops the reference on the previous material and takes one on the new material. A null
     * argument drops the previous material without storing the null, which is faithful to the
     * binary.
     *
     * @param pMat The material, or null to release the previous material alone.
     * @ghidraAddress 0x00493a78
     */
    void SetMaterial(Mat *pMat);

    /**
     * Point the mesh at the transform it draws with.
     *
     * Drops the reference on the previous owner and takes one on the new owner. A null argument
     * drops the previous owner without storing the null, which is faithful to the binary.
     *
     * @param pOwner The transform, or null to release the previous owner alone.
     * @ghidraAddress 0x00493c40
     */
    void SetTransOwner(Transformable *pOwner);

    /**
     * Test a ray against this mesh and append what it strikes to sink.
     *
     * Rnd::Collideable vtable slot 1. A bounding sphere with a non-zero radius rejects the ray
     * first, in world space. The faces are then tested in the local space of mTransOwner, so the
     * ray is brought there through the inverse of the owner's world transform rather than every
     * vertex being brought out. Each face of mFacesOwner is tested against the vertices of
     * mVertsOwner, with the cull mode of the material deciding whether a back-facing hit counts,
     * and a strike appends this mesh and the distance along the ray. The base implementation runs
     * last, so the children are tested after this mesh's own faces.
     *
     * @param ray The segment to test along.
     * @param sink The collector to append intersections to.
     * @ghidraAddress 0x0047f950
     */
    virtual void Collide(const Ray &ray, HitSink &sink);

protected:
    // The four overrides below fill Rnd::Drawable vtable slots 4 through 7. The access of those
    // base declarations is not recovered yet, and protected is the narrowest that admits the
    // Rnd::PsMesh overrides.

    /**
     * Rebuild whatever the platform subclass derives from the geometry.
     *
     * Empty in Rnd::Mesh. Rnd::PsMesh rebuilds its triangle strips here. The name is inferred
     * from the Rnd::Drawable vtable slot it fills.
     *
     * @ghidraAddress 0x00492770
     */
    virtual void Sync();

    /**
     * Report which parts of the mesh have changed.
     *
     * Empty in Rnd::Mesh and in Rnd::PsMesh. The name is inferred from the Rnd::Drawable vtable
     * slot it fills.
     *
     * @param nMask The changed parts.
     * @ghidraAddress 0x00492778
     */
    virtual void SyncChanged(int nMask);

    /**
     * Report every part of the mesh as changed.
     *
     * @ghidraAddress 0x00492780
     */
    virtual void SyncAll();

    /**
     * Restore the reference bookkeeping and resynchronise after a load or a copy.
     *
     * Adds a reference for each of the seven object references, then calls SyncAll() followed by
     * Sync(). The name is inferred from the Rnd::Drawable vtable slot it fills.
     *
     * @ghidraAddress 0x00493e10
     */
    virtual void Refresh();

private:
    // Take a reference on each object this mesh points at. 0x00493e10 inlines it as its own first
    // half, and Refresh() is its only caller.
    void AddObjectRefs();

    // Drop the reference on each object this mesh points at. The destructor, Load(), and Copy()
    // are its callers. 0x00493d48.
    void RemoveObjectRefs();

    // Empty the vertex vector when mVertsOwner is another mesh, and the face and edge vectors when
    // mFacesOwner is another mesh. Load() and Copy() are its callers. 0x0047fe68.
    void ClearSharedGeometry();

    // Data members follow the recovered offset order, and the access specifiers interleave.

protected:
    // Rnd::PsMesh::DrawSelf() reads both depth fields to build the GS register writes.
    ZMode mZMode; // +0xe0
    ZFunc mZFunc; // +0xe4

public:
    /*!< Vertices, owned when mVertsOwner is this mesh. Public because Rnd::Blur::RebuildBlurMesh()
         at 0x004b9b30 fills the vector directly and the image has no accessor for it. +0xe8 */
    std::vector<MeshVert> mVerts;
    /*!< Triangles, owned when mFacesOwner is this mesh. Public on the same evidence as mVerts, the
         builder at 0x004b9b30 writing all three pointers of the vector. +0xf4 */
    std::vector<MeshFace> mFaces;
    /*!< Drawn edges. Public because Rnd::Text::BuildGlyphMesh() at 0x004c9780 writes the vector
         directly. +0x100 */
    std::vector<MeshEdge> mEdges;

protected:
    // Rnd::PsMesh::DrawSelf() reads the material to select it and the sphere radius to cull.
    Mat *mMat;      // +0x10c
    Sphere mSphere; // +0x110

public:
    /*!< Mesh whose vertices this one draws, itself for a mesh that owns them. Public because the
         blur builder at 0x004b9b30 reads it to decide whether it may refill mVerts. +0x130 */
    Mesh *mVertsOwner;
    /*!< Mesh whose triangles and edges this one draws, itself for a mesh that owns them. Public on
         the same evidence as mVertsOwner. +0x134 */
    Mesh *mFacesOwner;

protected:
    // SetTransOwner() is the accessor for the first of the three, and the other two are written
    // only by a load or a copy. Rnd::PsMesh::DrawSelf() reads mTransOwner directly, which is what
    // keeps it out of the private section.
    Transformable *mTransOwner; // +0x138

private:
    Transformable *mTrans1Owner; // +0x13c
    Transformable *mTrans2Owner; // +0x140

protected:
    // Rnd::PsMesh::DrawSelf() reads the cap to decide how much of the mesh to submit.
    int mMaxVerts; // +0x144

private:
    float mMinScreen; // +0x148
    Mesh *mNext;      // +0x14c
};

/**
 * Allocate and construct a mesh.
 *
 * This is the creator the mesh class registers with Rnd::Manager, invoked through the hook below.
 *
 * @param name The object name.
 * @return The new mesh.
 * @ghidraAddress 0x00492ff0
 */
Mesh *NewMesh(const HxStr &name);

/**
 * Creator the registered "Mesh" class builds through.
 *
 * GfxDevice::Init() overwrites the hook with the Rnd::PsMesh creator, so a mesh loaded from a file
 * on the PlayStation 2 is a PsMesh. Rnd::Blur, HudDisplay, and Rnd::Tunnel also build their meshes
 * through it.
 *
 * @ghidraAddress 0x006eed60
 */
extern Mesh *(*g_pfnNewMesh)(const HxStr &name);

/**
 * Registered class name of Rnd::Mesh, the string "Mesh".
 *
 * @ghidraAddress 0x006eed68
 */
extern HxStr g_meshClassName;

/**
 * Serial version of the mesh record currently being read.
 *
 * Load() reads the version out of the file into this global at its start and then tests it
 * eighteen times, which is why no store to it appears in the routine. The store happens through
 * the pointer the stream receives. The word belongs to the mesh class rather than to a stream, and
 * the neighbouring words are the same thing for other classes; `0x00894d64` is the tunnel version
 * and `0x00894e2c` is the material version. The file-wide version Rnd::Manager::Read() consults at
 * `0x0089df90` is a separate mechanism that never interacts with this one.
 *
 * @ghidraAddress 0x00894d68
 */
extern int g_nRndMeshLoadVersion;

} // namespace Rnd
