#pragma once

#include "rndartt/acanvas.h"

class APalette;

/**
 * One stretched copy of a source bitmap into a destination rectangle, walked a row at a time.
 *
 * The record is not polymorphic and has no RTTI. ACanvas::ClipAndSetupScaledBitmap() builds it,
 * and the ACanvas::DrawScaledBitmap() family walks it and passes it to a scaled row slot once per
 * destination row.
 *
 * The record is 0x28 bytes. The first 0x1c bytes describe one destination row and are all a row
 * slot reads. The vertical walk follows. ACanvas::ClipAndSetupScaledBitmap() writes every member
 * except mY, and the row loop manages mY.
 *
 * The four per-format row slots differ only in the width they read mTransparentColor at. The
 * indexed form reads one byte, the 1555 form a halfword, and the 24 and 32 bit forms a word. The
 * member is modelled at its widest.
 *
 * mSourcePosition advances by mSourceStep once per destination column, and the whole part selects
 * the source byte. The row runs from mLeft up to but excluding mRight.
 *
 * The base row slots resolve a source index through the virtual pixel writer and never read
 * mPalette. ACanvasLin32::DrawScaledBitmapRowLin8U() and
 * ACanvasLin32::DrawScaledClutBitmapRowLin8U() read it directly instead and return when it is
 * null.
 */
struct ACanvas::AScaledRowInfo {
    int mSourcePosition;            /*!< The source offset in 24.8 fixed point. +0x00 */
    int mSourceStep;                /*!< The per column advance in 24.8 fixed point. +0x04 */
    unsigned char *mSource;         /*!< The source row. +0x08 */
    APalette *mPalette;             /*!< The palette the source indices belong to. +0x0c */
    unsigned int mTransparentColor; /*!< The source value to skip. +0x10 */
    short mHasTransparentColor;     /*!< Whether mTransparentColor applies. +0x14 */
    short mY;                       /*!< The destination row. +0x16 */
    short mLeft;                    /*!< The first destination column. +0x18 */
    short mRight;                   /*!< One past the last destination column. +0x1a */
    short mTop;           /*!< The first destination row, clamped to the clip rectangle. +0x1c */
    short mBottom;        /*!< One past the last destination row, clamped likewise. +0x1e */
    int mSourcePositionY; /*!< The source row in 24.8 fixed point. +0x20 */
    int mSourceStepY;     /*!< The per row advance in 24.8 fixed point. +0x24 */
};
