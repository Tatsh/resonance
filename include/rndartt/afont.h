#pragma once

#include "rndartt/abitmap.h"

/**
 * Bitmap font of one glyph image per character code.
 *
 * The record is not polymorphic and has no RTTI, and the image retains no title for it. The name
 * here is inferred from the two routines that consume it, ACanvas::DrawGlyphNoClip() and
 * ACanvas::DrawTextNoClip(). It is unrelated to Rnd::Font, which draws through a material atlas
 * rather than through a canvas.
 *
 * The five members below are the ones the two consumers read. The total size is not settled,
 * because no allocation of the record was located.
 *
 * Each glyph is an ABitmap, and DrawGlyphNoClip() selects the copy routine from the glyph format
 * code rather than from any member here. A glyph therefore needs no common format across the font.
 * The advance of one character comes from the glyph mWidth, so the font is proportional.
 */
struct AFont {
    ABitmap **mGlyphs;    /*!< One glyph image per code from mFirstCharCode. +0x00 */
    short mGlyphCount;    /*!< The number of entries in mGlyphs. +0x04 */
    short mUnknown06;     /*!< Purpose undetermined. +0x06 */
    short mFirstCharCode; /*!< The character code mGlyphs[0] draws. +0x08 */
    short mLineHeight;    /*!< The row advance of a newline. +0x0a */
    short mBaseline;      /*!< Rows between the drawing origin and the glyph top. +0x0c */
};
