#pragma once

/**
 * One kind of part the FreQ maker offers, shared by every FreqPart placed from it.
 *
 * The class is not polymorphic and emits no RTTI descriptor, and no literal in the image
 * identifies it, so the name is inferred. MetFreqMakerAssetManager stores the templates in a
 * vector indexed by identifier and vends them through MetFreqMakerAssetManager::GetPart(). The
 * constructor and the size are not recovered, so only the two members FreqPart reads are declared.
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
    unsigned char mUnknown02[0xe]; // +0x02

public:
    /**
     * Non-zero when a part of this kind takes a colour. +0x10
     *
     * Public because FreqPart::SetColor() reads it directly, and the image has no accessor.
     */
    int mColorable;
};
