#pragma once

#include "os/hxstr.h"

namespace Rnd {
class Mat;
}

/**
 * One kind of part the FreQ maker offers, shared by every FreqPart placed from it.
 *
 * The class is not polymorphic and emits no RTTI descriptor, and no literal in the image
 * identifies it, so the name is inferred. The object is 0x24 bytes, which the allocation in
 * MetFreqMakerAssetManager::RegisterPart() fixes. That routine builds one template for each part
 * texture of the FreQ maker asset file, and MetFreqMakerAssetManager keeps them in a vector
 * indexed by identifier, in a map by name, and in one list for each category.
 *
 * The constructor and the destructor are emitted in the translation unit that begins at
 * `0x00255790`, beside template library code, although every caller is in
 * MetFreqMakerAssetManager's unit.
 *
 * Every member is public, because MetFreqMakerAssetManager, FreqPart, and FreqAppearanceDetail read
 * and write them directly and the image has no accessor.
 */
class FreqPartTemplate {
public:
    /**
     * Record a part texture's name, material, and bitmap size.
     *
     * The identifier starts at -1 and the colour, randomisation, and category words at 0.
     *
     * @param name The name of the part texture.
     * @param pMaterial The material built over the texture.
     * @param nScaleX The texture's bitmap width.
     * @param nScaleZ The texture's bitmap height.
     * @ghidraAddress 0x002576c0
     */
    FreqPartTemplate(const HxStr &name, Rnd::Mat *pMaterial, int nScaleX, int nScaleZ);

    /**
     * Release the name.
     *
     * The destructor is not virtual. MetFreqMakerAssetManager::ReleaseParts() deletes templates
     * through its deleting form.
     *
     * @ghidraAddress 0x00257730
     */
    ~FreqPartTemplate();

    /**
     * The identifier GetPart() indexes by, the template's position in the asset file. FreqPart
     * transfers only its low half. +0x00
     */
    int mId;

    /** The name FindPart() looks the template up by, the part texture's name. +0x04 */
    HxStr mName;

    /** The material a mesh of this kind is drawn with. +0x0c */
    Rnd::Mat *mMaterial;

    /** Non-zero when a part of this kind takes a colour. +0x10 */
    int mColorable;

    /** Non-zero when FreqAppearanceDetail::randomize() may change a part of this kind. +0x14 */
    int mRandomizable;

    /**
     * The category the template is listed under, 1 through 11, or 0 for none. +0x18
     *
     * FreqAppearanceDetail::nudgeCursor() also writes it.
     */
    int mCategory;

    /** The texture's bitmap width, which scales a mesh of this kind in 1/128 units. +0x1c */
    int mScaleX;

    /** The texture's bitmap height, which scales a mesh of this kind in 1/128 units. +0x20 */
    int mScaleZ;
};
