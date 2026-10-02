#pragma once

#include "rndartt/apoint.h"

/**
 * One side of a convex polygon, walked a row at a time by the ACanvas polygon fills.
 *
 * The record is not polymorphic and has no RTTI, and the image retains no title for it. The name
 * here follows the `A` prefix every other art library type uses and the routines that fill it,
 * APolygon::SetupEdge() and APolygon::SetupTexturedEdge(), and is therefore inferred rather than
 * recovered.
 *
 * The record is 0x20 bytes. Both fills place their two edges 0x20 bytes apart on the stack, and
 * the textured setup writes the last member at 0x1c. The flat fill leaves the texture members
 * unwritten.
 *
 * mTop and mBottom are destination rows and bound the rows the edge covers, from mTop up to but
 * excluding mBottom. mX and both texture members are 24.8 fixed point.
 */
struct APolygonEdge {
    short mFrom;      /*!< The polygon vertex the edge starts at. +0x00 */
    short mTo;        /*!< The polygon vertex the edge ends at. +0x02 */
    int mX;           /*!< The column on the current row. +0x04 */
    int mStepX;       /*!< The column advance per row. +0x08 */
    short mTop;       /*!< The first row the edge covers. +0x0c */
    short mBottom;    /*!< One past the last row the edge covers. +0x0e */
    APoint mTexCoord; /*!< The texel position on the current row. +0x10 */
    APoint mTexStep;  /*!< The texel advance per row. +0x18 */
};
