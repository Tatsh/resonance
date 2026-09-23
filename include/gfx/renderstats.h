#pragma once

/**
 * Per-frame counters the renderer maintains and the debug overlay reports.
 *
 * Each counter is recovered from the routine that increments it. mUnknown0c has no recovered
 * writer, and the total size is unrecovered. The overlay that reads the block is at `0x0049c0c0`.
 */
struct RenderStats {
    /** Point particles submitted, advanced by the point path by the whole vertex count. */
    int mnPoints;
    /** Meshes submitted, incremented by Rnd::PsMesh::DrawSelf(). */
    int mnMeshDraws;
    /** Triangles with at least one vertex outside a plane, counted before the drop tests. */
    int mnFacesClipped;
    int mUnknown0c;
    /** Triangles submitted, advanced by Rnd::PsMesh::DrawFacesVU1() by the face count. */
    int mnTriangles;
    /** Edges dropped by the clip test. */
    int mnEdgesClipped;
    /** Lines emitted, which stands to the edge count as mnTriangles does to the face count. */
    int mnLines;
    /** Sprite pairs rejected whole by the corner test. */
    int mnSpritesCulled;
    /** Sprites drawn, advanced per sprite and by the batch count in the VU1 path. */
    int mnSpritesDrawn;
    /** Vertices transformed, advanced by both vertex transform passes by the run length. */
    int mnVertsTransformed;
    /** GIF tags opened, incremented by GfxDevice::WriteGifTag(). */
    int mnGifTags;
    /** GIF packets submitted, incremented by GfxDevice::FlushGifPacket(). */
    int mnGifPackets;
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
