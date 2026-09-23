#pragma once

#include "os/hxstr.h"

namespace Rnd {
class Mat;
}

/**
 * One kind of part the FreQ maker offers, shared by every FreqPart placed from it.
 *
 * The class is not polymorphic and emits no RTTI descriptor, and no literal in the image
 * identifies it, so the name is inferred. MetFreqMakerAssetManager stores the templates in a
 * vector indexed by identifier and vends them through MetFreqMakerAssetManager::GetPart(). The
 * constructor and the size are not recovered, so only the members other classes read are declared.
 */
class FreqPartTemplate {
public:
    /**
     * The identifier GetPart() indexes by. +0x00
     *
     * Public because FreqPart::Pack() reads it directly, and the image has no accessor.
     */
    short mId;

private:
    unsigned char mUnknown02[0x2]; // +0x02

public:
    /**
     * The name MetFreqMakerAssetManager::FindPart() looks the template up by. +0x04
     *
     * Public because FreqAppearanceDetail::save() writes it directly, and the image has no
     * accessor.
     */
    HxStr mName;

    /**
     * The material a mesh of this kind is drawn with. +0x0c
     *
     * Public because FreqAppearanceDetail::unpack() reads it directly, and the image has no
     * accessor.
     */
    Rnd::Mat *mMaterial;

    /**
     * Non-zero when a part of this kind takes a colour. +0x10
     *
     * Public because FreqPart::SetColor() reads it directly, and the image has no accessor.
     */
    int mColorable;

    /**
     * Non-zero when FreqAppearanceDetail::randomize() may change a part of this kind. +0x14
     *
     * Public because FreqAppearanceDetail::randomize() reads it directly, and the image has no
     * accessor.
     */
    int mRandomizable;

    /**
     * The category the template is listed under, 1 through 11. +0x18
     *
     * Public because FreqAppearanceDetail reads it directly, and the image has no accessor.
     * FreqAppearanceDetail::nudgeCursor() also writes it.
     */
    int mCategory;

    /**
     * The x and z scale of a mesh of this kind, in 1/128 units. +0x1c and +0x20
     *
     * Public because MetFreqMakerAssetManager::ApplyPartScale() reads them directly, and the image
     * has no accessor.
     */
    int mScaleX;
    int mScaleZ; /*!< See mScaleX. */
};
