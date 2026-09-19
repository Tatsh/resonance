#pragma once

#include "rndartt/apalette.h"

/**
 * One destination row sampled from a source row at a fractional rate.
 *
 * The record is not polymorphic and has no RTTI, and the image retains no title for it. The name
 * here is inferred from the six routines that consume it, ACanvas::StretchRowIndexed() through
 * ACanvas::StretchRowBlend().
 *
 * The record is 0x1c bytes. Every member below appears in at least one consumer, and the four
 * per format consumers differ only in the width they read mTransparentColor at. The indexed form
 * reads one byte, the 1555 form a halfword, and the 24 and 32 bit forms a word, so the member is
 * modelled at its widest.
 *
 * mSourcePosition advances by mSourceStep once per destination column, and the whole part selects
 * the source byte. The row runs from mLeft up to but excluding mRight.
 *
 * The base implementations resolve a source index through the virtual pixel writer and never read
 * mPalette. ACanvasLin32::StretchRowIndexed() and ACanvasLin32::StretchRowRemap() read it
 * directly instead and return when it is null, which is how the member was recovered.
 */
struct AStretchSpan {
    int mSourcePosition;            /*!< The source offset in 24.8 fixed point. +0x00 */
    int mSourceStep;                /*!< The per column advance in 24.8 fixed point. +0x04 */
    unsigned char *mSource;         /*!< The source row. +0x08 */
    APalette *mPalette;             /*!< The palette the source indices belong to. +0x0c */
    unsigned int mTransparentColor; /*!< The source value to skip. +0x10 */
    short mHasTransparentColor;     /*!< Whether mTransparentColor applies. +0x14 */
    short mY;                       /*!< The destination row. +0x16 */
    short mLeft;                    /*!< The first destination column. +0x18 */
    short mRight;                   /*!< One past the last destination column. +0x1a */
};
