#pragma once

#include <list>

#include "game/freqmakercursor.h"
#include "game/freqpart.h"
#include "math/color.h"
#include "math/vector2.h"
#include "os/hxstr.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

namespace Rnd {
class View;
}

/**
 * Avatar an appearance stores apart from its username.
 *
 * The class is not polymorphic and emits no RTTI descriptor, so no accessor and no vtable names it.
 * Nor does the lever of last resort reach it. FreqAppearance's copy constructor takes its 0xb0
 * bytes from the untagged allocator at `0x004a81e0`, which passes no class name, and the two tagged
 * allocations inside its own constructor are billed to `stl_list` and to `Rnd::View`. The title
 * here is inferred from its one owner and from the diagnostic literal ` Freq=` that
 * FreqAppearance::Print writes where this object would appear.
 *
 * The object is 0xb0 bytes. FreqMakerCursor is its non-virtual base at `+0x00` through `+0x9f`, the
 * part list is at `+0xa0`, and the view every part mesh hangs from is at `+0xa4`. Besides
 * FreqAppearance, MetFreqMakerCanvasScreen edits the avatar through this class.
 *
 * The translation unit spans `0x00249b48` to `0x0024f798`.
 *
 * No identifier here is attested by the image, so every member takes the required style rather than
 * the CamelCase the classes around it use. That divergence is deliberate.
 */
class FreqAppearanceDetail : public FreqMakerCursor {
public:
    /**
     * Start with an empty part list and a fresh view named by the FreQ maker asset manager.
     *
     * @ghidraAddress 0x00249c40
     */
    FreqAppearanceDetail();

    /**
     * Build a fresh avatar and copy another one's parts in.
     *
     * The image has no caller.
     *
     * @param other The avatar to copy.
     * @ghidraAddress 0x00249e58
     */
    FreqAppearanceDetail(const FreqAppearanceDetail &other);

    /**
     * Discard every part and delete the view.
     *
     * The destructor is not virtual. FreqAppearance's own destructor releases this object through
     * a direct call rather than through a table.
     *
     * @ghidraAddress 0x0024edc8
     */
    ~FreqAppearanceDetail();

    /**
     * Write the avatar to a stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x0024b160
     */
    void save(OBStream &stream);

    /**
     * Read the avatar back from a stream.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x0024b340
     */
    void load(IBStream &stream);

    /**
     * Delete every part, empty the view, and reset the placement state.
     *
     * The destructor runs this before releasing the view, and FreqAppearance::operator=() runs it
     * before copying the source in. The name is inferred from those two uses.
     *
     * @ghidraAddress 0x0024a4b0
     */
    void clear();

    /**
     * Append a copy of each of another avatar's parts, in the other view's draw order.
     *
     * The other avatar's parts are first indexed by mesh in a local std::map, and each drawable
     * of the other view other than its preview mesh is then looked up and copied. The routine
     * does not clear first, so FreqAppearance::operator=() pairs it with clear(). The name is
     * inferred from that pairing.
     *
     * @param other The avatar to copy.
     * @ghidraAddress 0x0024a088
     */
    void copyFrom(const FreqAppearanceDetail &other);

    /**
     * Report the parts the avatar is built from.
     *
     * Defined in the header. The out-of-line copy is called by FreqAppearance::RenderBurnTextures()
     * and by the routine at `0x00262270`.
     *
     * @return The part list.
     * @ghidraAddress 0x0024f230
     */
    std::list<FreqPart *> &parts() {
        return mParts;
    }

    /**
     * Pack every part into consecutive 8-byte records.
     *
     * The part list is also counted a second time and that count is discarded.
     * FreqAppearance::Pack() is the one caller.
     *
     * @param pOut The first record to write.
     * @param pCount Receives the number of parts written.
     * @ghidraAddress 0x0024c398
     */
    void pack(FreqPart::Packed *pOut, int *pCount);

    /**
     * Append parts unpacked from consecutive 8-byte records.
     *
     * For each record the body allocates a FreqPart, unpacks it, clones a mesh for it with the
     * template's material and scale, places the mesh at the part's position raised to a height of
     * 0.1, hangs it from mView, mirrors it when the part is mirrored, colours the part from the
     * palette, records the scale steps, appends the part, and resets the placement state.
     * FreqAppearance::Unpack() is the one caller.
     *
     * @param pRecords The first record to read.
     * @param nCount The number of records.
     * @ghidraAddress 0x0024b898
     */
    void unpack(const FreqPart::Packed *pRecords, int nCount);

    /**
     * Create the preview mesh if there is none, hanging it from mView, and clear three placement
     * words.
     *
     * The title is inferred.
     *
     * @ghidraAddress 0x00249b48
     */
    void ensureCursorMesh();

    /**
     * Hide the preview mesh at the origin with no material and return the placement state to its
     * unset values.
     *
     * unpack() expands the same body. The title is inferred.
     *
     * @ghidraAddress 0x0024ee60
     */
    void resetCursor();

    /**
     * Start placing a new part of one template.
     *
     * The placement state is reset, and with fewer than 16 parts the named template is shown on
     * the preview mesh with its material, scale, and the current colour, and the preview mesh is
     * moved to the end of the view's lists. The title is inferred.
     *
     * @param name The template name.
     * @ghidraAddress 0x0024a5f0
     */
    void selectTemplate(const HxStr &name);

    /**
     * Place the previewed part, or stop editing the selected one.
     *
     * While placing, a copy of the preview mesh becomes a new part at the cursor, drawn just
     * before the preview mesh, with the current colour and mirroring, and the placement state is
     * reset. While editing, the selection is dropped and placing resumes. The title is inferred.
     *
     * @ghidraAddress 0x0024a770
     */
    void placeCursor();

    /**
     * Move the cursor one unit, moving the preview mesh or the selected part along with it.
     *
     * Each axis moves only while the part's edges stay inside -55 to 55, and a part of category 1
     * does not rise above zero. Nothing moves while placing with 16 parts. The title is inferred.
     *
     * @param nStepX -1, 0, or 1 along x.
     * @param nStepZ -1, 0, or 1 along z.
     * @ghidraAddress 0x0024a9e8
     */
    void nudgeCursor(int nStepX, int nStepZ);

    /**
     * Start editing one part, retaining a copy to revert to.
     *
     * The title is inferred.
     *
     * @param nIndex The part's position in the part list.
     * @return The part, or null for an index outside the list.
     * @ghidraAddress 0x0024ae20
     */
    FreqPart *selectPart(int nIndex);

    /**
     * Reset the placement state and delete one part.
     *
     * The title is inferred.
     *
     * @param nIndex The part's position in the part list. An index outside the list deletes
     *               nothing.
     * @ghidraAddress 0x0024bc50
     */
    void deletePart(int nIndex);

    /**
     * Restore the selected part's colour, mirroring, draw position, and placement from the copy
     * selectPart() retained, and drop the selection.
     *
     * The title is inferred.
     *
     * @ghidraAddress 0x0024be00
     */
    void revertSelection();

    /**
     * Give each part that may be randomised a random change.
     *
     * With a chance of a third a part takes a random palette colour, and with another third a
     * random template of its category, unless the category is 1, 2, or 8. The title is inferred.
     *
     * @ghidraAddress 0x0024bfc8
     */
    void randomize();

    /**
     * Move one part and the cursor to the origin and bring the part to the front.
     *
     * The title is inferred.
     *
     * @param nIndex The part's position in the part list.
     * @ghidraAddress 0x0024c218
     */
    void recentrePart(int nIndex);

    /**
     * Move the previewed or selected mesh one step earlier in the draw order.
     *
     * The title is inferred.
     *
     * @ghidraAddress 0x0024ef08
     */
    void sendBackward();

    /**
     * Move the previewed or selected mesh one step later in the draw order.
     *
     * The title is inferred.
     *
     * @ghidraAddress 0x0024ef80
     */
    void bringForward();

    /**
     * Report where a part's mesh sits in the view's draw order.
     *
     * revertSelection() and selectPart() expand the same body. The out-of-line copy has no
     * caller. The title is inferred.
     *
     * @param pPart The part.
     * @return The position, or the number of drawables when the mesh is not drawn.
     * @ghidraAddress 0x0024eff8
     */
    int drawIndexOf(FreqPart *pPart);

    /**
     * Mirror the previewed or selected part.
     *
     * The title is inferred.
     *
     * @ghidraAddress 0x0024f060
     */
    void toggleMirror();

    /**
     * Record the current colour and apply it to the previewed or selected part.
     *
     * The title is inferred.
     *
     * @param color The colour.
     * @param palettePosition The palette position the colour came from.
     * @ghidraAddress 0x0024f178
     */
    void setColor(const Color &color, const Vector2 &palettePosition);

    /**
     * Hang mView from a parent view.
     *
     * The title is inferred.
     *
     * @param pParent The parent view.
     * @ghidraAddress 0x0024f238
     */
    void attachTo(Rnd::View *pParent);

    /**
     * Unhang mView from a parent view.
     *
     * The title is inferred.
     *
     * @param pParent The parent view.
     * @ghidraAddress 0x0024f290
     */
    void detachFrom(Rnd::View *pParent);

private:
    // Report the part at one position, or the last part when the list is shorter. Four routines
    // expand the same walk.
    FreqPart *partAt(int nIndex);

    // Move the previewed or selected mesh through the draw order. sendBackward() and
    // bringForward() expand the same body with opposite steps.
    void moveCursorDraw(int nSteps);

    std::list<FreqPart *> mParts; // +0xa0

public:
    /**
     * The avatar view the constructor allocates, 0x120 bytes. +0xa4
     *
     * Public because FreqAppearance::AttachToBurnSlot() reads it directly, and the image has no
     * accessor for it.
     */
    Rnd::View *mView;

private:
    unsigned char mUnknowna8[0x8]; // +0xa8
};
