#pragma once

#include "rndartt/abitmap.h"

/**
 * Bits ACanvas::ClipCodeForPoint() returns.
 *
 * The four bits form a Cohen and Sutherland outcode against the canvas clip rectangle. A point
 * inside the rectangle produces zero.
 */
enum ACanvasClipCode {
    kACanvasClipLeft = 1,  /*!< The point sits left of the clip rectangle. */
    kACanvasClipRight = 2, /*!< The point sits at or right of the clip rectangle. */
    kACanvasClipAbove = 4, /*!< The point sits above the clip rectangle. */
    kACanvasClipBelow = 8  /*!< The point sits at or below the clip rectangle. */
};

/**
 * Drawing surface over an ABitmap, with a clip rectangle.
 *
 * The name comes from the RTTI descriptor at 0x0086f660, whose mangled form is `7ACanvas`. The
 * descriptor records no base class. The virtual function table pointer sits after the data
 * members at offset 0x20, the position the toolchain uses for a class with no base.
 *
 * The table at 0x00837dc8 has 85 slots and terminates on the all zero entry at 0x00838070. Slot 0
 * is the type function at 0x005ead28 and slot 1 is the destructor. 22 of the remaining slots point
 * at the shared pure virtual stub at 0x005381a8, and the rest are drawing operations the image
 * does not identify. An earlier enumeration of this table stopped at slot 63 and missed slots 64
 * through 84. Those 21 slots are now titled in the program.
 *
 * Five subclasses implement the drawing operations, one per accepted ABitmapFormat code, and each
 * derives through an intermediate width class: ACanvasLin4 through ACanvas8, ACanvasLin8 through
 * ACanvas8, ACanvasLin15 through ACanvas15, ACanvasLin24 through ACanvas24, and ACanvasLin32
 * through ACanvas32. Every object in the family is 0x28 bytes, the subclass field at offset 0x24
 * being one byte wide for the two indexed formats, two for the 15 bit format, and four for the 24
 * and 32 bit formats.
 *
 * Three further members are recovered but not written here.
 *
 * `CreateForBitmap(const ABitmap &bitmap, bool bAllocatePixels)` at 0x005e8bc8 copies the
 * description, and when asked to allocate derives the row stride from the format code. Code 0
 * gives `(mWidth + 2) / 2` and every other code multiplies mWidth by the byte width in the table
 * at 0x00725cc0, whose first six entries are 0, 1, 2, 3, 4, and 1. The pixel rectangle is then
 * allocated with the tag "abitmap.h" and line 0x47. The format code finally selects the subclass
 * to construct through the jump table at 0x00837d90, and a code of 6 or more yields null.
 *
 * `CreateWithOwnedPixels(const ABitmap &bitmap)` at 0x005eb200 rewrites a format code of
 * kABitmapFormatUnknown5 to kABitmapFormatLinear8 and then calls CreateForBitmap() with
 * allocation requested.
 *
 * `ClipBlitSpan` at 0x005eb418 clips one source row against the clip rectangle and writes the
 * start and end columns plus the destination row into three small output records. Its six
 * arguments are recovered. The records they address are not, and the signature is therefore
 * unsettled.
 */
class ACanvas {
public:
    /**
     * Adopt a pixel description.
     *
     * The description is copied whole and the clip rectangle is set to the full bitmap, from the
     * origin to mWidth by mHeight. Every subclass constructor calls this first and then installs
     * its own virtual function table.
     *
     * @param bitmap The description to adopt.
     * @ghidraAddress 0x005eb1a0
     */
    explicit ACanvas(const ABitmap &bitmap);

    /**
     * Release the canvas.
     *
     * The compiled body reinstalls this class's virtual function table before releasing. That is
     * what the toolchain emits for a base destructor. The pixel rectangle is not released here.
     *
     * @ghidraAddress 0x005ead68
     */
    virtual ~ACanvas();

    /**
     * Classify a point against the clip rectangle.
     *
     * @param nX The horizontal coordinate.
     * @param nY The vertical coordinate.
     * @return The ACanvasClipCode bits, or zero when the point is inside.
     * @ghidraAddress 0x005eb3d0
     */
    int ClipCodeForPoint(int nX, int nY) const;

    /**
     * Read the pixel at a point, or zero when the point falls outside the clip rectangle.
     *
     * The body tests the point against all four clip bounds and returns zero for a point outside
     * them. For a point inside it forwards to the virtual slot immediately before this one, which
     * is the per-format read, and returns the value that read produced. In this class the earlier
     * slot is the shared pure virtual stub. ACanvasLin32 implements it at 0x00614350 as a word
     * load from the pixels at the row stride times the row plus four times the column, which
     * settles the result as one 32-bit word. Its signedness is not recoverable.
     *
     * The routine occupies slot 32 of the 85 slot table, and its position in this declaration does
     * not reproduce that index. Slots 2 through 31 have no titles yet, so nothing exists to declare
     * ahead of it.
     *
     * @param nX The horizontal coordinate.
     * @param nY The vertical coordinate.
     * @return The pixel, or zero when the point is clipped away.
     * @ghidraAddress 0x005eb948
     */
    virtual unsigned GetPixel(int nX, int nY);

    ABitmap mBitmap;   /*!< The pixel rectangle this canvas draws into. +0x00 */
    short mClipLeft;   /*!< The first column the clip rectangle admits. +0x18 */
    short mClipTop;    /*!< The first row the clip rectangle admits. +0x1a */
    short mClipRight;  /*!< One past the last column the clip rectangle admits. +0x1c */
    short mClipBottom; /*!< One past the last row the clip rectangle admits. +0x1e */
};
