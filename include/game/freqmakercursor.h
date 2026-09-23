#pragma once

#include "game/freqpart.h"
#include "math/color.h"
#include "math/vector2.h"

class FreqPartTemplate;
namespace Rnd {
class Mesh;
}

/**
 * Placement state of the FreQ maker, the part being placed and the mesh that previews it.
 *
 * The class is not polymorphic and emits no RTTI descriptor, and no literal in the image
 * identifies it, so the name is inferred. It is the non-virtual base of FreqAppearanceDetail at
 * offset 0 and occupies `+0x00` through `+0x9f`, which the detail's part list at `+0xa0` fixes. The
 * detail's constructor runs this constructor first on the same object, and the detail's destructor
 * runs this destructor last with an `__in_chrg` of 2. Every member is protected, because only the
 * detail reads them.
 */
class FreqMakerCursor {
public:
    /**
     * Start with no template selected, an unset scale, and the FreQ maker default colour.
     *
     * @ghidraAddress 0x0024f300
     */
    FreqMakerCursor();

    /**
     * Delete the preview mesh.
     *
     * @ghidraAddress 0x0024f3b0
     */
    ~FreqMakerCursor();

protected:
    int mUnknown00;                // +0x00
    int mUnknown04;                // +0x04
    unsigned char mUnknown08[0x8]; // +0x08
    Color mColor;                  // +0x10, starts as g_freqMakerDefaultColor
    int mUnknown20;                // +0x20
    unsigned char mUnknown24[0xc]; // +0x24
    FreqPart mPart;                // +0x30
    int mUnknown70;                // +0x70
    FreqPartTemplate *mTemplate;   // +0x74, the template being placed, or null
    int mUnknown78;                // +0x78, starts at 1
    float mScaleX;                 // +0x7c, the template's x scale, 1000 while unset
    float mScaleZ;                 // +0x80, the template's z scale, 1000 while unset
    int mScaleStepX;               // +0x84, mScaleX in 1/110 steps, -1 while unset
    int mScaleStepZ;               // +0x88, mScaleZ in 1/110 steps, -1 while unset
    int mUnknown8c;                // +0x8c
    Vector2 mPalettePosition;      // +0x90, starts at (-1, -1)
    Rnd::Mesh *mCursorMesh;        // +0x98, the preview mesh, owned
};
