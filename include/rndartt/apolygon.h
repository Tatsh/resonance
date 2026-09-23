#pragma once

struct ABitmap;
struct APoint;
struct APolygonEdge;

/** Vertex slots an APolygon provides in APolygon::mIndices. */
constexpr int kAPolygonMaxVertexCount = 8;

/**
 * Convex polygon for the ACanvas polygon fills, as vertex indices into a shared point array.
 *
 * The record is not polymorphic and has no RTTI, and the image retains no title for it. The name
 * here follows the `A` prefix every other art library type uses, and is therefore inferred rather
 * than recovered. Its three routines sit inside the ACanvas translation unit, between the ACanvas
 * members that call them.
 *
 * The record is at least 0x18 bytes. The index array runs from 0x0c to the texture coordinate
 * pointer at 0x14, which bounds it at eight vertices. No reader of the halfword at 0x04 was
 * located.
 *
 * The first word has two readings. ACanvas::FillPolygon() passes it to ACanvas::SetColorNative(),
 * and ACanvas::FillTexturedPolygon() passes it as the source bitmap of
 * ACanvas::TextureRowIndexed(). It is modelled as a union of the two.
 *
 * Every position is 24.8 fixed point. A vertex position is mPoints[mIndices[i]], while a texture
 * coordinate is mTexCoords[i] with no indirection. Each texture coordinate is a fraction of the
 * texture extent, because APolygon::SetupTexturedEdge() multiplies it by the texture width or
 * height.
 */
struct APolygon {
    /**
     * Return the position in mIndices of the vertex with the smallest row.
     *
     * A tie resolves to the earlier position.
     *
     * @return The index into mIndices.
     * @ghidraAddress 0x005ebfe0
     */
    int FindTopVertex() const;

    /**
     * Start an edge at one vertex and aim it at a neighbour.
     *
     * The neighbour is nFrom plus nDirection, wrapped to the vertex count at either end. The
     * previous mBottom of the edge becomes its mTop, and the new mBottom is the neighbour's row
     * rounded to the nearest whole row with g_nFixedHalf. The column step is the column
     * difference divided by the row count, or zero for an edge of no rows.
     *
     * @param pEdge The edge to fill.
     * @param nFrom The vertex position in mIndices the edge starts at.
     * @param nDirection 1 to walk forward through mIndices, -1 to walk backward.
     * @ghidraAddress 0x005e97c8
     */
    void SetupEdge(APolygonEdge *pEdge, short nFrom, int nDirection) const;

    /**
     * Start an edge as SetupEdge() does and add its texture walk.
     *
     * Scales both texture coordinates of the edge by the texture extent. The texture step is
     * written only for an edge of one row or more, so an empty edge retains the previous step.
     *
     * @param pEdge The edge to fill.
     * @param nFrom The vertex position in mIndices the edge starts at.
     * @param nDirection 1 to walk forward through mIndices, -1 to walk backward.
     * @param pTexture The texture whose extent scales the coordinates.
     * @ghidraAddress 0x005e9bd8
     */
    void SetupTexturedEdge(APolygonEdge *pEdge,
                           short nFrom,
                           int nDirection,
                           const ABitmap *pTexture) const;

    union {
        unsigned int mColor;     /*!< The fill colour in the canvas pixel format. +0x00 */
        const ABitmap *mTexture; /*!< The texture of a textured fill. +0x00 */
    };
    short mUnknown04;                                /*!< No reader was located. +0x04 */
    short mVertexCount;                              /*!< Vertices in mIndices. +0x06 */
    const APoint *mPoints;                           /*!< The shared vertex positions. +0x08 */
    unsigned char mIndices[kAPolygonMaxVertexCount]; /*!< Positions into mPoints. +0x0c */
    const APoint *mTexCoords; /*!< Texture coordinates, one per vertex. +0x14 */
};
