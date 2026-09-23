#pragma once

#include <cstddef>
#include <map>

#include "os/hxstr.h"
#include "rnd/object.h"

class FailSink;
namespace Rnd {
class Mat;
class Stream;
} // namespace Rnd
struct Vector2;

namespace Rnd {

/**
 * Which of the two descriptions a font supplies.
 *
 * The three titles come from the printer at `0x004d07e0`, which writes "Default", "Builtin", and
 * "Material" for the values below. A Builtin font describes a host font by height, weight, italic
 * flag, family, and face name. A Material font is a glyph atlas inside a Rnd::Mat texture, and it
 * is the only kind Rnd::Text builds geometry for.
 */
enum FontType { kFontTypeDefault = 0, kFontTypeBuiltin = 1, kFontTypeMaterial = 2 };

/**
 * Stroke weight of a Builtin font.
 *
 * The three titles come from the table of pointers at `0x006fecc0`, which the printer at
 * `0x004d0858` indexes with mWeight and hands straight to FailSink::Print() with no bound check.
 * A fourth word follows the three and reads zero. The family table begins at the next quadword
 * boundary, so that word is as likely to be alignment padding as a null element, and the source
 * below writes three entries.
 */
enum FontWeight { kFontWeightThin = 0, kFontWeightNormal = 1, kFontWeightBold = 2 };

/**
 * Typeface family of a Builtin font.
 *
 * The four titles come from the table of pointers at `0x006fecd0`, which the printer at
 * `0x004d0898` indexes with mFamily.
 */
enum FontFamily {
    kFontFamilyModern = 0,
    kFontFamilyRoman = 1,
    kFontFamilySwiss = 2,
    kFontFamilyScript = 3
};

/**
 * Glyph atlas, either a host font description or a grid of cells inside a material texture.
 *
 * `Q23Rnd4Font` in the RTTI descriptor at `0x008ef4c0`, with `Rnd::Object` as its only public
 * non-virtual base at offset 0. The class is 0x60 bytes, which the factory at `0x004cf060` pins by
 * requesting exactly that many. The Rnd::Object vptr therefore sits at `+0x18` and this class's
 * members run from `+0x1c` to `+0x5f` with no padding. The ten-entry vtable is at `0x008225d8`,
 * eight Rnd::Object slots followed by the pair of Builtin accessors this class declares.
 *
 * The member titles come from the text DumpText() writes, "type:", " height:", " weight:",
 * "italic:", " family:", " name:", " mat:", " rows:", " cols:", "size:", and " space:".
 *
 * A Material font divides the first texture of its material into mRows by mCols cells of equal
 * size and assigns the characters of mChars to those cells in reading order. BuildCharMap() then
 * measures every glyph and records the result in mCharMap, so a character absent from mChars has no
 * metrics, no advance, and a collapsed quad.
 */
class Font : public Object {
public:
    /**
     * Allocate a font under the tag "Rnd::Font".
     *
     * @param nSize The object size the compiler supplies.
     * @return The block.
     * @ghidraAddress 0x004cecc0
     */
    static void *operator new(size_t nSize);

    /**
     * Release a font to the tagged heap.
     *
     * @param pBlock The block.
     * @ghidraAddress 0x004cece0
     */
    static void operator delete(void *pBlock);

    /**
     * Measured metrics of one glyph.
     *
     * The title is inferred; the record is a plain value with no RTTI and no allocation tag of its
     * own. It is 0x14 bytes of five floats, which the 0x28-byte red-black tree node of mCharMap
     * pins (0x10 bytes of node header, a 4-byte key, then this record) and which the five words
     * ComputeCharUV() zeroes for a font with no texture confirm independently.
     *
     * The two texture corners are stored rather than a corner and an extent. The tail of
     * ComputeCharUV() adds the cell extent to the first corner through the two-component vector
     * addition at `0x00169818` and stores the sum into the last two fields, and GetCharUV() hands
     * the two pairs out unchanged as the two corners of the glyph quad.
     *
     * Every value is normalised. The corners are texture coordinates in the range 0 to 1, and the
     * advance is already scaled by mSize, so it arrives in the units Rnd::Text builds its mesh in
     * rather than in pixels.
     */
    struct CharInfo {
        float mAdvance; /*!< Horizontal advance, mSize scaled by the measured glyph width as a
                             fraction of its cell. +0x00 */
        float mU0;      /*!< Left texture coordinate of the glyph. +0x04 */
        float mV0;      /*!< Top texture coordinate of the glyph. +0x08 */
        float mU1;      /*!< Right texture coordinate of the glyph. +0x0c */
        float mV1;      /*!< Bottom texture coordinate of the glyph. +0x10 */
    };

    /**
     * Construct a Default font with a one-cell grid.
     *
     * The height starts at 12, the weight at Normal, the family at Roman, the italic flag clear,
     * both grid dimensions at 1.0, and the cell size and the tracking at 0.0. Neither string is
     * allocated and no material is attached, so a font built this way measures nothing until it is
     * loaded or copied.
     *
     * @param name The registry key for this object.
     * @ghidraAddress 0x004cb1e8
     */
    explicit Font(const HxStr &name);

    /**
     * Drop the reference on the material and release the character map.
     *
     * @ghidraAddress 0x004cee70
     */
    virtual ~Font();

    /**
     * Write a description of this font to sink.
     *
     * The base description comes first. Everything below is produced only at a positive dump level,
     * and which fields follow the class tag depends on mType: a Builtin font writes the host font
     * description and a Material font writes the atlas, while a Default font writes neither.
     *
     * @param sink The diagnostic sink to write to.
     * @ghidraAddress 0x004ca470
     */
    virtual void DumpText(FailSink &sink);

    /**
     * Write this font to stream at revision 2.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x004ca720
     */
    virtual void Save(Stream &stream);

    /**
     * Retarget the material when it is the object being replaced.
     *
     * @param pFrom The object being replaced.
     * @param pTo The replacement, or null.
     * @ghidraAddress 0x004d0690
     */
    virtual void Replace(Object *pFrom, Object *pTo);

    /**
     * Report the class key a `.rnd` file writes for a font.
     *
     * The returned string is the global at `0x006fecb0`, which the class registration fills with
     * "Font".
     *
     * @return The class key.
     * @ghidraAddress 0x004cefd0
     */
    virtual const HxStr &ClassName() const;

    /**
     * Copy the state of pSource into this font.
     *
     * Every field except the object name is copied and the character map is then emptied. nFlags is
     * not read, and no base implementation is invoked. A pSource that is not a Font is dereferenced
     * through the null the cast produces rather than rejected.
     *
     * @param pSource The font to copy from.
     * @param nFlags Unread.
     * @ghidraAddress 0x004cb0b8
     */
    virtual void Copy(const Object *pSource, unsigned nFlags);

    /**
     * Read this font from stream.
     *
     * A revision above 2 produces the report "Can't load new Font" followed by the abort handler of
     * g_failSink. Revision 0 reads a character map of its own, with a 12-byte record rather than
     * the 20-byte one this build uses, and then discards every entry; such a file supplies no
     * material and no atlas either, and all five of those fields survive at their constructed
     * values. Revision 0 and revision 1 both take the default character set rather than reading
     * one, and revision 1 stores the two grid dimensions as integers.
     *
     * The character map is emptied at the end whatever the revision, so the next measurement
     * rebuilds it against the material that has just arrived.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x004cac20
     */
    virtual void Load(Stream &stream);

    /**
     * Describe a Builtin host font.
     *
     * Vtable slot 8. The five fields it writes are the ones a Builtin font dumps and serialises.
     * No direct call to this routine survives, and only a virtual dispatch through slot 8 could
     * reduce that to a negative finding about the whole image.
     *
     * @param nHeight The height.
     * @param weight The stroke weight.
     * @param nItalic Non-zero for an italic face.
     * @param family The typeface family.
     * @param name The face name.
     * @ghidraAddress 0x004cedd0
     */
    virtual void
    SetBuiltin(int nHeight, FontWeight weight, int nItalic, FontFamily family, const HxStr &name);

    /**
     * Report the Builtin host font description.
     *
     * Vtable slot 9, and the exact inverse of SetBuiltin().
     *
     * @param nHeightOut Receives the height.
     * @param weightOut Receives the stroke weight.
     * @param nItalicOut Receives the italic flag.
     * @param familyOut Receives the typeface family.
     * @param nameOut Receives the face name.
     * @ghidraAddress 0x004cef88
     */
    // No direct call to slot 9 survives either. Whatever consumed a Builtin font description is
    // outside the part of the image this tree has recovered.
    virtual void GetBuiltin(int *nHeightOut,
                            FontWeight *weightOut,
                            int *nItalicOut,
                            FontFamily *familyOut,
                            HxStr &nameOut);

    /**
     * Report the horizontal advance of one character.
     *
     * Builds the character map first when it is empty. A character absent from the map advances by
     * nothing at all rather than by a fallback width.
     *
     * @param ch The character to measure.
     * @return The advance, in the units Rnd::Text builds its mesh in.
     * @ghidraAddress 0x004d0988
     */
    float GetCharAdvance(char ch);

    /**
     * Report the two texture-coordinate corners of one character.
     *
     * Builds the character map first when it is empty. A character absent from the map yields two
     * zero corners, which collapses its quad rather than drawing a substitute glyph.
     *
     * @param ch The character to look up.
     * @param uv0 Receives the left and top corner.
     * @param uv1 Receives the right and bottom corner.
     * @ghidraAddress 0x004d08d8
     */
    void GetCharUV(char ch, Vector2 &uv0, Vector2 &uv1);

    /**
     * Measure every character of mChars and record its metrics in mCharMap.
     *
     * Walks mChars in order, assigning each character the next cell of the mRows by mCols grid and
     * wrapping to the next row after mCols cells. A character appearing twice in mChars is measured
     * twice and the second measurement overwrites the first, so the later cell wins.
     *
     * @ghidraAddress 0x004ca978
     */
    void BuildCharMap();

    /**
     * Replace the glyph atlas material.
     *
     * Drops the reference on the previous material, records the new one, and then runs
     * OnChanged(), which takes the new reference and discards the measured metrics. The title is
     * inferred.
     *
     * @param pMat The new material, or null.
     * @ghidraAddress 0x004d0600
     */
    void SetMat(Mat *pMat);

    /**
     * Change the height one cell occupies.
     *
     * Follows the same sequence as SetMat(), dropping and retaking the material reference around
     * the store and discarding the measured metrics. The title is inferred.
     *
     * @param flSize The new mSize.
     * @ghidraAddress 0x004d0648
     */
    void SetSize(float flSize);

private:
    // Take this object's reference on mMat and empty mCharMap, so the metrics are measured again
    // on the next lookup. SetMat(), SetSize(), and the setter at 0x004d0530 are the callers. The
    // title is inferred. 0x004d0768.
    void OnChanged();

    /**
     * Measure the glyph occupying one cell of the atlas.
     *
     * Locks the first texture of mMat, scans the cell column by column for the first and the last
     * column with any pixel whose alpha byte is set, and derives the advance and the two corners
     * from the span it finds. A cell that is entirely transparent yields a span of a quarter of the
     * cell, which is what gives a space character its width.
     *
     * A font with no material, with an empty stage vector, or with no texture bitmap yields five
     * zeroes instead.
     *
     * @param nRow The cell row.
     * @param nCol The cell column.
     * @param infoOut Receives the metrics.
     * @ghidraAddress 0x004ca050
     */
    void ComputeCharUV(int nRow, int nCol, CharInfo &infoOut);

    // Drop this object's reference on mMat. The destructor is its only out-of-line caller, and
    // Replace(), Load(), and Copy() inline the same body. 0x004d0738.
    void RemoveMatRef();

    // Declared in recovered offset order, with the access specifiers interleaved.

public:
    /** Which of the two descriptions this font supplies. Public because
        Rnd::Text::BuildGlyphMesh() at `0x004c9780` tests it through a Rnd::Font pointer from
        outside this hierarchy, refusing to build geometry for anything but a Material font, and
        the image exposes no accessor for it. +0x1c */
    FontType mType;

private:
    // Height, weight, italic flag, family, and face name of a Builtin font. Private because only
    // this class touches any of the five, through SetBuiltin(), GetBuiltin(), and the four
    // serialisation members.
    int mHeight;        // +0x20
    FontWeight mWeight; // +0x24
    int mItalic;        // +0x28
    FontFamily mFamily; // +0x2c
    HxStr mName;        // +0x30

    // Measured metrics, one entry per distinct character of mChars, keyed by the character itself.
    // The key is read with a signed byte load at every comparison site, so a character above 127
    // sorts before every ASCII one.
    //
    // Seven of the tree operations are instantiated out of line, and all seven are library code
    // that this tree writes no body for. For this map they are the insert helper at 0x004cdc50,
    // insert() at 0x004ce040, the hinted insert at 0x004ce180, find() at 0x004ceb28, lower_bound()
    // at 0x004d0d70, and the recursive subtree erase at 0x004d0d00. Load() builds a second,
    // throwaway map of the revision 0 record over the same key type, whose insert helper is at
    // 0x004ce318, whose insert() is at 0x004ce6e8, whose hinted insert is at 0x004ce828, and whose
    // subtree erase is at 0x004d0db0.
    std::map<char, CharInfo> mCharMap; // +0x38

public:
    /** Material whose first texture supplies the glyph atlas. +0x44 */
    Mat *mMat;

    /** Cell rows of the glyph atlas. +0x48 */
    float mRows;

    /** Cell columns of the glyph atlas. +0x4c */
    float mCols;

    /** Height one cell occupies, in the units Rnd::Text builds its mesh in. +0x50 */
    float mSize;

    /** Tracking added after every glyph, in the same units as mSize. +0x54 */
    float mSpace;

private:
    // Characters the atlas supplies, in the reading order of its cells.
    HxStr mChars; // +0x58
};

/**
 * Allocate and construct a font.
 *
 * The binary bills the allocation to the tag "Rnd::Font" and requests exactly 0x60 bytes.
 *
 * @param name The object name.
 * @return The new font.
 * @ghidraAddress 0x004cf060
 */
Font *NewFont(const HxStr &name);

/**
 * Creator the registered "Font" class builds through.
 *
 * RegisterFontClass() points it at NewFont(), and Rnd::Manager::Init() writes it a second time at
 * `0x00519d44`.
 *
 * @ghidraAddress 0x006fecb8
 */
extern Font *(*g_pfnNewFont)(const HxStr &name);

/**
 * Build a font through g_pfnNewFont.
 *
 * The one recovered reference to this routine is its entry in the exception range table, and
 * nothing in the image calls it. The title follows NewTextThroughHook(), its counterpart in the
 * Text half of the unit.
 *
 * @param name The object name.
 * @return The new font.
 * @ghidraAddress 0x004ced40
 */
Font *NewFontThroughHook(const HxStr &name);

/**
 * Build a font for the registered "Font" class.
 *
 * Calls through g_pfnNewFont. Rnd::Object is the non-virtual base at offset 0, so the result
 * needs no adjustment.
 *
 * @param name The object name.
 * @return The new font, as its Rnd::Object base.
 * @ghidraAddress 0x004cefe0
 */
Object *CreateRegisteredFont(const HxStr &name);

/**
 * Point g_pfnNewFont at NewFont() and register the "Font" class with Rnd::Manager.
 *
 * The image has no caller. The name is inferred from RegisterTextClass().
 *
 * @ghidraAddress 0x004ced00
 */
void RegisterFontClass();

/**
 * Registered class name of Rnd::Font, the string "Font".
 *
 * @ghidraAddress 0x006fecb0
 */
extern HxStr g_fontClassName;

} // namespace Rnd
