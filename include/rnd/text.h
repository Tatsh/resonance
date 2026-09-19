#pragma once

#include "math/color.h"
#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/collideable.h"
#include "rnd/drawable.h"
#include "rnd/font.h"
#include "rnd/mesh.h"
#include "rnd/object.h"
#include "rnd/stream.h"
#include "rnd/transformable.h"

namespace Rnd {

/**
 * Bits of the Rnd::Text alignment word.
 *
 * The six titles and the six bit values come from the printer at `0x004d0450`, which tests the
 * three vertical bits first and then the three horizontal ones, writing at most one of each.
 * Nothing enforces that a word has one bit from each group.
 */
enum TextAlign {
    kTextAlignLeft = 0x01,   /*!< Lines start at the origin. */
    kTextAlignCenter = 0x02, /*!< Lines are centred on the origin. */
    kTextAlignRight = 0x04,  /*!< Lines end at the origin. */
    kTextAlignTop = 0x10,    /*!< The first line sits at the origin. */
    kTextAlignMiddle = 0x20, /*!< The block is centred on the origin. */
    kTextAlignBottom = 0x40  /*!< The last line sits at the origin. */
};

/**
 * Run of text drawn as a quad per glyph.
 *
 * `Q23Rnd4Text` in the RTTI descriptor at `0x008ef4d0`, with three public non-virtual bases whose
 * offsets the descriptor fixes: `Rnd::Drawable` at `+0x00`, `Rnd::Collideable` at `+0x14`, and
 * `Rnd::Transformable` at `+0x20`. All three derive virtually from `Rnd::Object`, so one shared
 * Object subobject sits at `+0x110`, which the constructor proves by writing that address into all
 * three virtual-base pointers.
 *
 * The class is 0x130 bytes, which the factory at `0x004cf800` pins by requesting exactly that
 * many. The last member ends at `0x12c`, and the quadword access to mColor gives the class 16-byte
 * alignment, which accounts for the remaining four bytes. The same alignment accounts for the
 * eight bytes between mZTest and the base subobject, since `0x108` is not a 16-byte boundary and
 * `0x110` is. Neither run is an unrecovered field.
 *
 * Four vtables belong to the class, laid out back to back and each terminated by an all-zero
 * entry. The three-entry table at `0x00822508` is addressed by the Rnd::Transformable vptr with a
 * `-0x20` adjustment, the three-entry table at `0x00822528` by the Rnd::Collideable vptr with a
 * `-0x14` adjustment, the eight-entry table at `0x00822548` by the Rnd::Drawable vptr with no
 * adjustment, and the eight-entry table at `0x00822590` by the Object subobject vptr with a
 * `-0x110` adjustment. Rnd::Drawable is the primary base, so the four virtuals this class declares
 * of its own occupy slots 4 through 7 of its table.
 *
 * The member titles come from the text DumpText() writes, "font:", " align:", "preWrapText:",
 * "color:", " word wrap:", " wrap width:", "text:", and "mesh:".
 *
 * Geometry is built rather than drawn directly. The object owns one Rnd::Mesh whose name is the
 * object's own inside `[` and `_mesh]`, has four vertices and two triangles per glyph, and takes
 * its material from the font. Every writer of a field that affects layout ends by rebuilding that
 * mesh, and a Text whose font is not a Material font builds nothing at all.
 *
 * Wrapping retains two strings. mPreWrapText is what a caller or a file supplied, and mText is the
 * same text with a newline inserted at every wrap point. mText is the string the mesh is built
 * from, and it is derived rather than stored, so only mPreWrapText is serialised.
 */
class Text : public Drawable, public Collideable, public Transformable {
public:
    /**
     * Construct an empty white text run.
     *
     * The colour starts fully opaque white, the alignment at Top and Left, the wrap width at
     * 100.0, wrapping disabled, and both strings, the font, and the mesh empty.
     *
     * @param name The registry key for this object.
     * @ghidraAddress 0x004c89c0
     */
    explicit Text(const HxStr &name);

    /**
     * Release the glyph mesh and drop the reference on the font.
     *
     * @ghidraAddress 0x004cf2b8
     */
    virtual ~Text();

    /**
     * Set the colour of every glyph.
     *
     * Rnd::Drawable vtable slot 4. The new colour is written over the colour of every vertex the
     * glyph mesh already stores and the mesh is told that its colours changed, so the change takes
     * effect without a rebuild.
     *
     * @param color The colour.
     * @ghidraAddress 0x004cfec8
     */
    virtual void SetColor(const Color &color);

    /**
     * Set which point of the block the origin is.
     *
     * Rnd::Drawable vtable slot 5.
     *
     * @param nAlign A set of the TextAlign bits.
     * @ghidraAddress 0x004cff48
     */
    virtual void SetAlign(int nAlign);

    /**
     * Replace the text.
     *
     * Rnd::Drawable vtable slot 6. The argument becomes mPreWrapText, and mText and the glyph mesh
     * are derived from it.
     *
     * @param text The text to draw.
     * @ghidraAddress 0x004d0168
     */
    virtual void SetText(const HxStr &text);

    /**
     * Replace the font.
     *
     * Rnd::Drawable vtable slot 7. A null argument is stored, unlike Rnd::Mesh::SetMaterial(),
     * which drops the previous material without storing the null.
     *
     * @param pFont The font, or null for none.
     * @ghidraAddress 0x004cfde0
     */
    virtual void SetFont(Font *pFont);

    /**
     * Set whether long lines are broken at word boundaries.
     *
     * @param nWordWrap Non-zero to wrap.
     * @ghidraAddress 0x004cfab8
     */
    void SetWordWrap(int nWordWrap);

    /**
     * Set the width a wrapped line is broken at.
     *
     * @param flWrapWidth The width, in the units the font measures in.
     * @ghidraAddress 0x004cfb78
     */
    void SetWrapWidth(float flWrapWidth);

    /**
     * Derive mText from mPreWrapText and rebuild the glyph mesh.
     *
     * Wrapping is applied only when it is enabled and a font is attached, since wrapping needs
     * glyph widths to measure with. Every setter of a field that affects layout ends here, and the
     * compiler inlined this routine at each of those call sites while retaining the out-of-line
     * body for Load() and Copy().
     *
     * @ghidraAddress 0x004cf9f8
     */
    void RebuildText();

    /**
     * Write a description of this text run to sink.
     *
     * The Rnd::Object description comes first and then each of the three mix-ins, because
     * Rnd::Object is a virtual base and the mix-ins therefore do not write it themselves.
     * Everything below is produced only at a positive dump level, and mText and the mesh only at a
     * dump level of 2 or more.
     *
     * @param sink The diagnostic sink to write to.
     * @ghidraAddress 0x004c7fc0
     */
    virtual void DumpText(FailSink &sink);

    /**
     * Write this text run to stream at revision 6.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x004c8330
     */
    virtual void Save(Stream &stream);

    /**
     * Retarget the font when it is the object being replaced.
     *
     * Each of the three mix-ins is given the pair first.
     *
     * @param pFrom The object being replaced.
     * @param pTo The replacement, or null.
     * @ghidraAddress 0x004cf888
     */
    virtual void Replace(Object *pFrom, Object *pTo);

    /**
     * Report the class key a `.rnd` file writes for a text run.
     *
     * The returned string is the global at `0x006feca0`, which the class registration fills with
     * "Text".
     *
     * @return The class key.
     * @ghidraAddress 0x004cf760
     */
    virtual const HxStr &ClassName() const;

    /**
     * Copy the state of pSource into this text run.
     *
     * The colour is not among the fields copied, so a copy draws in whatever colour this object
     * already had. A pSource that is not a Text is dereferenced through the null the cast produces
     * rather than rejected.
     *
     * @param pSource The text run to copy from.
     * @param nFlags The set of fields to copy, passed on to each mix-in.
     * @ghidraAddress 0x004cfca8
     */
    virtual void Copy(const Object *pSource, unsigned nFlags);

    /**
     * Read this text run from stream.
     *
     * A revision above 6 produces the report "Can't load new Text" followed by the abort handler of
     * g_failSink. The older revisions differ in five ways. Below revision 3 the alignment arrives
     * as an index into a table of six words rather than as the bit set. Below revision 2 the
     * transform is absent and a plain x and y pair stands in for it, becoming the translation row
     * of the local transform with the y component negated and scaled by three quarters. Below
     * revision 1 the colour is absent. Below revision 4 wrapping is absent and the wrap width
     * returns to 100.0. Revision 5 alone writes mText, which this build derives instead, and
     * revision 5 introduced the depth-test flag.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x004c8560
     */
    virtual void Load(Stream &stream);

    /**
     * Test a ray against the glyph mesh and append what it strikes to sink.
     *
     * Rnd::Collideable vtable slot 1. Every intersection the mesh records is retargeted at this
     * object, so a caller receives the text run rather than the mesh it happens to own. A text run
     * that is not showing is not tested at all.
     *
     * @param ray The segment to test along.
     * @param sink The collector to append intersections to.
     * @ghidraAddress 0x004c7ef8
     */
    virtual void Collide(const Ray &ray, HitSink &sink);

    /**
     * Set the billboard mode of this object and of its glyph mesh.
     *
     * Rnd::Transformable vtable slot 1.
     *
     * @param nBillboard The billboard mode.
     * @ghidraAddress 0x004d02a0
     */
    virtual void SetBillboard(int nBillboard);

    /**
     * Compose the world transform of this object and then of its glyph mesh.
     *
     * Rnd::Transformable vtable slot 2. The mesh is recomposed against this object, and the base
     * report becomes the force argument of that second call, so the mesh follows unconditionally
     * whenever this object moved.
     *
     * @param pParent The transformable this one hangs off, or null for a root.
     * @param nForce Non-zero to recompose even when nothing is marked dirty.
     * @return Non-zero when the world transform was recomposed.
     * @ghidraAddress 0x004d03e0
     */
    virtual int UpdateWorldXfm(Transformable *pParent, int nForce);

    /**
     * Set whether this object draws at all.
     *
     * Rnd::Drawable vtable slot 1. Clearing the flag releases the glyph mesh outright rather than
     * hiding it, and setting it rebuilds that mesh. A value equal to the current one is discarded,
     * and the base implementation is not invoked.
     *
     * @param nShowing Non-zero to draw.
     * @ghidraAddress 0x004d0378
     */
    virtual void SetShowing(int nShowing);

    /**
     * Set whether this object draws with its highlight treatment.
     *
     * Rnd::Drawable vtable slot 2. The glyph mesh receives the same flag.
     *
     * @param nHighlight Non-zero to highlight.
     * @ghidraAddress 0x004d0328
     */
    virtual void SetHighlight(int nHighlight);

protected:
    /**
     * Draw the glyph mesh.
     *
     * Rnd::Drawable vtable slot 3. Always reports that the children are still to be drawn, even
     * when there is no mesh.
     *
     * @return Non-zero, always.
     * @ghidraAddress 0x004d02f8
     */
    virtual int DrawSelf();

private:
    /**
     * Rebuild the glyph mesh from mText.
     *
     * Releases whatever mesh exists first, then builds nothing at all unless this object is showing
     * and its font is a Material font. Otherwise it creates a mesh through the Rnd::Mesh creator
     * hook, names it after this object, sizes its vertex and face vectors for four vertices and two
     * triangles per glyph, and emits one line at a time.
     *
     * @ghidraAddress 0x004c9780
     */
    void BuildGlyphMesh();

    /**
     * Emit the quads of one line into the glyph mesh.
     *
     * The two floats arrive in the first two single-precision argument registers and the three
     * integers in the first three integer ones, which is the EE convention for mixed arguments and
     * is what fixes the declaration order below.
     *
     * @param flLineY Index of the line, counting down from the first.
     * @param flLineWidth Total advance of the line, which the horizontal alignment offsets by.
     * @param nCharBase Number of glyphs already emitted, which indexes both vectors.
     * @param pBegin First character of the line.
     * @param pEnd One past the last character of the line.
     * @ghidraAddress 0x004c9ca0
     */
    void EmitLineGlyphs(
        float flLineY, float flLineWidth, int nCharBase, const char *pBegin, const char *pEnd);

    /**
     * Insert a newline at every wrap point of text.
     *
     * The text is copied into a stack buffer, broken line by line, and returned. A text whose first
     * line already fits is returned unchanged, and a text whose very first character is a newline
     * is returned with no wrapping applied at all, because FindLineBreak() reports nothing for such
     * a line and the routine treats that as having nothing to do.
     *
     * @param text The text to wrap.
     * @return The wrapped text.
     * @ghidraAddress 0x004c95d0
     */
    HxStr ApplyWordWrap(const HxStr &text);

    /**
     * Report how many characters of a line fit inside mWrapWidth.
     *
     * Measures the run up to the next newline first and accepts the whole line when it fits.
     * Failing that it measures the first word, and if even that overflows it shortens the count one
     * character at a time until the remainder fits. Otherwise it adds one word at a time while the
     * running measurement still fits.
     *
     * @param pText The line to measure, which has to be NUL-terminated.
     * @return The number of characters that fit, or 0 when the line starts with a newline.
     * @ghidraAddress 0x004c9278
     */
    int FindLineBreak(const char *pText);

    // Total advance of the first nCount characters of pText, truncated to a whole number. The
    // compiler inlined this at all five measurement sites of FindLineBreak(), which is its only
    // caller. A text run with no font measures nothing.
    float MeasureRun(const char *pText, int nCount);

    // Drop the reference on the font and release the glyph mesh. The destructor is its only
    // out-of-line caller, and Copy() and BuildGlyphMesh() inline the same body. 0x004cf958.
    void RemoveObjectRefs();

    // Take a reference on the font and rebuild. Copy() and Load() inline the same body.
    // 0x004cf9b8.
    void AddObjectRefs();

    // Declared in recovered offset order. Every member is private: each one that the engine changes
    // has a setter, no call from outside this class arrives at one of those setters, and the image
    // no accessor for any of them.

    Color mColor;  // +0xd0
    int mAlign;    // +0xe0
    Font *mFont;   // +0xe4
    int mWordWrap; // +0xe8
    // Width a wrapped line is broken at. Load() clamps a value read from a file below revision 5
    // into 0 through 1000, and nothing clamps a value SetWrapWidth() receives.
    float mWrapWidth; // +0xec
    // mPreWrapText with a newline inserted at every wrap point. Derived rather than stored, which
    // is why Save() does not write it.
    HxStr mText;        // +0xf0
    HxStr mPreWrapText; // +0xf8
    // Geometry built from mText. Owned outright, created through the Rnd::Mesh creator hook, and
    // marked internal so that a scene save does not write it as an object of its own.
    Mesh *mMesh; // +0x100
    // Whether the glyph mesh reads the depth buffer. Set gives the mesh kZModeZReadOnly with
    // kZFuncLess, and clear gives it kZModeDisable with kZFuncNever. The title is inferred from
    // that pair, since the image supplies no string for the field and no setter for it either.
    int mZTest; // +0x104

    // Alignment padding rather than an unrecovered field. The quadword access to mColor gives the
    // class 16-byte alignment, and the virtual base subobject therefore starts at 0x110.
    unsigned char mReserved108[0x08];
};

/**
 * Allocate and construct a text run.
 *
 * The binary bills the allocation to the tag "Rnd::Text" and requests exactly 0x130 bytes.
 *
 * @param name The object name.
 * @return The new text run.
 * @ghidraAddress 0x004cf800
 */
Text *NewText(const HxStr &name);

/**
 * Creator the registered "Text" class builds through.
 *
 * @ghidraAddress 0x006feca8
 */
extern Text *(*g_pfnNewText)(const HxStr &name);

/**
 * Build a text run through the creator hook.
 *
 * The one recovered reference to this routine is the data word at `0x00869c28`, and nothing in the
 * image calls it, so what consumed it did not ship.
 *
 * @param name The object name.
 * @return The new text run.
 * @ghidraAddress 0x004cf1d0
 */
Text *NewTextThroughHook(const HxStr &name);

/**
 * Build a text run for the registered "Text" class.
 *
 * Calls through g_pfnNewText and narrows the result to its Rnd::Object subobject, which is why the
 * routine exists at all rather than the hook being registered directly.
 *
 * @param name The object name.
 * @return The new text run, as its Rnd::Object subobject.
 * @ghidraAddress 0x004cf770
 */
Object *CreateRegisteredText(const HxStr &name);

/**
 * Point g_pfnNewText at NewText() and register the "Text" class with Rnd::Manager.
 *
 * @ghidraAddress 0x004cf190
 */
void RegisterTextClass();

/**
 * Registered class name of Rnd::Text, the string "Text".
 *
 * @ghidraAddress 0x006feca0
 */
extern HxStr g_textClassName;

} // namespace Rnd
