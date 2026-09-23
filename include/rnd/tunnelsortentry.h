#pragma once

#include "math/vector3.h"
#include "rnd/mesh.h"
#include "rnd/meshvert.h"

namespace Rnd {

/**
 * A mesh of a Rnd::Tunnel keyed by its squared distance from an eye point.
 *
 * The record is 0xc bytes and has no RTTI. Its only trace is the out-of-line copy of its
 * constructor, which sits in the Rnd::Tunnel unit and has no callers, so the name is inferred
 * from what the constructor computes.
 */
struct TunnelSortEntry {
    /**
     * Key a mesh by the squared distance from eye to the midpoint of its first two vertices.
     *
     * The vertices are those of the mesh's mVertsOwner.
     *
     * @param pMesh The mesh.
     * @param nIndex A word stored alongside.
     * @param eye The eye point.
     * @ghidraAddress 0x00476930
     */
    TunnelSortEntry(Mesh *pMesh, int nIndex, const Vector3 &eye) : mMesh(pMesh), mIndex(nIndex) {
        const std::vector<MeshVert> &verts = pMesh->mVertsOwner->mVerts;
        Vector3 sum;
        sum.w = 1.0f;
        AddVec3(&verts[0].mPoint.x, &verts[1].mPoint.x, &sum.x);
        Vector3 midpoint;
        midpoint.w = 1.0f;
        Vec3Scale(&sum.x, 0.5f, &midpoint.x);
        const float flX = midpoint.x - eye.x;
        const float flY = midpoint.y - eye.y;
        const float flZ = midpoint.z - eye.z;
        mDistanceSq = flX * flX + flY * flY + flZ * flZ;
    }

    float mDistanceSq; /*!< The squared distance from the eye. */
    Mesh *mMesh;       /*!< The mesh. */
    int mIndex;        /*!< The word stored alongside. */
};

} // namespace Rnd
