#pragma once

/**
 * Per-frame counters the renderer maintains and the debug overlay reports.
 *
 * Only the three counters below are recovered, each from the routine that increments it, so the
 * gaps are placeholders rather than settled fields and the total size is unrecovered. The overlay
 * that reads the block is at `0x0049c0c0`.
 */
struct RenderStats {
    int mUnknown00;
    /** Meshes submitted, incremented by Rnd::PsMesh::DrawSelf(). */
    int mnMeshDraws;
    int mUnknown08;
    int mUnknown0c;
    /** Triangles submitted, advanced by Rnd::PsMesh::DrawFacesVU1() by the face count. */
    int mnTriangles;
    int mUnknown14;
    int mUnknown18;
    int mUnknown1c;
    int mUnknown20;
    /** Vertices transformed, advanced by both vertex transform passes by the run length. */
    int mnVertsTransformed;
    int mUnknown28;
    int mUnknown2c;
    /** Materials applied, incremented by each of the Rnd::PsMat select entry points. */
    int mnMatSelects;
    /** Advanced by Rnd::TransformAndLightMeshVerts(). What it counts is undetermined. */
    int mUnknown34;
};

/**
 * The renderer's counters.
 *
 * @ghidraAddress 0x006f3940
 */
extern RenderStats g_renderStats;
