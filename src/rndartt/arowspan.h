#pragma once

class APalette;

/**
 * One row of source pixels addressed to one destination row of a canvas.
 *
 * The record is not polymorphic and has no RTTI, and the image retains no title for it. The name
 * here is inferred from the two routines that consume it, ACanvas::RemapRowIndexed() and
 * ACanvas::BlendRowIndexed().
 *
 * The record is 0x14 bytes. ACanvas::BlitRemap4() builds one on its stack and fills every member
 * except the padding byte, which settles both the layout and the order. The row runs from mLeft up
 * to but excluding mRight.
 *
 * Only ACanvas::BlitRemap4() writes mPalette, and no consumer recovered so far reads it. Its type
 * follows the value stored, which is the source palette, then the canvas palette, then the global
 * default palette addressed by the pointer at 0x0086f6f0.
 */
struct ARowSpan {
    short mY;                       /*!< The destination row. +0x00 */
    short mLeft;                    /*!< The first destination column. +0x02 */
    short mRight;                   /*!< One past the last destination column. +0x04 */
    bool mHasTransparentColor;      /*!< Whether mTransparentColor applies. +0x06 */
    unsigned char mPad07;           /*!< Padding the builder does not write. +0x07 */
    unsigned int mTransparentColor; /*!< The source value to skip. +0x08 */
    unsigned char *mSource;         /*!< The source pixels, one byte per pixel. +0x0c */
    APalette *mPalette;             /*!< The palette the source indices belong to. +0x10 */
};
