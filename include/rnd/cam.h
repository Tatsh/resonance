#pragma once

namespace Rnd {

/** Plane equations in a camera's draw frustum. */
constexpr int kCamFrustumPlaneCount = 6;

/** Floats per plane equation, a quadword each. */
constexpr int kCamFrustumPlaneFloatCount = 4;

/**
 * Camera.
 *
 * The class is not reconstructed. Only the two fields the draw path reads are declared, at the
 * offsets those readers use, and the runs between them are reserved rather than modelled. A
 * reserved run is a record of what has not been recovered, not a field.
 *
 * Rnd::Mesh::PrepareDraw() also reads `+0x80`, `+0x114`, `+0x120`, and `+0x300` for its
 * projected-size estimate. Those sit inside the reserved runs and are left there until the layout
 * is established.
 */
class Cam {
public:
    unsigned char mReserved00[0x280];
    /** Frustum Rnd::Mesh::PrepareDraw() tests against, distinct from Rnd::g_afDrawFrustumPlanes. */
    float mafFrustumPlanes[kCamFrustumPlaneCount][kCamFrustumPlaneFloatCount];
    unsigned char mReserved2e0[0x28];
    /** Non-zero to suppress the depth-register writes of Rnd::PsMesh::DrawSelf(). +0x308 */
    int mnSuppressDepthRegs;
};

/**
 * Camera the frame is being drawn through.
 *
 * Rnd::PsCam::Begin() stores its own camera argument here, which is what establishes the type.
 *
 * @ghidraAddress 0x006f9588
 */
extern Cam *g_pCurrentCam;

} // namespace Rnd
