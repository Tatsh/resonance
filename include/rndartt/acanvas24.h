#pragma once

#include "rndartt/acanvas.h"

/**
 * Colour conversion for a canvas of 24-bit pixels.
 *
 * The name comes from the RTTI descriptor at 0x008f0910, whose mangled form is `9ACanvas24` and
 * whose single public base is ACanvas at offset zero.
 *
 * THE CLASS IS ABSTRACT, and the set it leaves pure differs from every sibling's. Its table at
 * 0x00840a28 runs the same 85 entries as ACanvas's, so it adds no virtual, and it fills 18 of the
 * base's pure slots while leaving slots 12, 13, 19, and 29 pointing at the shared pure-virtual
 * stub: the alpha builder, the colourless store, and the two channel-triple pixel accessors. Its
 * layout subclass is ACanvasLin24, whose descriptor records ACanvas24 as its base.
 *
 * WHICH FOUR IT LEAVES IS THE DESIGN OF THE WHOLE FAMILY, and it is exact. A format class
 * implements every colour format OTHER than its own, by converting into its own, and leaves its own
 * format's pixel pair to whichever subclass knows the memory layout. This class leaves the
 * channel-triple pair at 19 and 29, ACanvas15 leaves the 1555 pair at 17 and 27, and ACanvas8
 * leaves the indexed pair at 15 and 25. Each also leaves slot 13, the colourless store. The alpha
 * builder at slot 12 is left by this class and by ACanvas15 and supplied by ACanvas8, which can do
 * it because an indexed format owns a palette whose entries can be rewritten.
 *
 * THE COLOUR MEMBER IS ACCESSED BOTH AS A WORD AND AS BYTES, so it is modelled as a union. The
 * native setter at 0x00630388 stores a whole word at `+0x24` with one instruction, while the index
 * setter at 0x00630448 writes three separate bytes at `+0x24`, `+0x25`, and `+0x26` and never
 * touches `+0x27`. A single word store would have written that fourth byte, so the two accesses
 * are genuinely different in the original rather than one being the compiler's rendering of the
 * other.
 *
 * No body is written yet. Every body is recoverable, and the four pure slots are not this class's
 * to supply.
 */
class ACanvas24 : public ACanvas {
public:
    /**
     * Construct over a bitmap.
     *
     * @param bitmap The bitmap the canvas addresses.
     * @ghidraAddress 0x006302f8
     */
    explicit ACanvas24(const ABitmap &bitmap);

    /** Slot 1. @ghidraAddress 0x00630278 */
    virtual ~ACanvas24();

    /** Slot 2. Expands a palette entry into the three channel bytes. @ghidraAddress 0x00630448 */
    virtual void SetColorIndex(int nIndex);

    /** Slot 3. @ghidraAddress 0x00630330 */
    virtual void SetColor15(unsigned short nColor);

    /** Slot 4. @ghidraAddress 0x00630360 */
    virtual void SetColorRGB(const unsigned char *pRGB);

    /** Slot 5. @ghidraAddress 0x00630380 */
    virtual void SetColor32(unsigned int nColor);

    /** Slot 6. Stores the whole word. @ghidraAddress 0x00630388 */
    virtual void SetColorNative(unsigned int nColor);

    /** Slot 7. @ghidraAddress 0x00630108 */
    virtual int GetColorIndex();

    /** Slot 8. @ghidraAddress 0x00630390 */
    virtual unsigned short GetColor15();

    /** Slot 9. @ghidraAddress 0x006303c8 */
    virtual void GetColorRGB(unsigned char *pRGB);

    /** Slot 10. @ghidraAddress 0x006303e8 */
    virtual unsigned int GetColor32();

    /** Slot 11. Reads the whole word. @ghidraAddress 0x006303f0 */
    virtual unsigned int GetColorNative();

    /** Slot 15. @ghidraAddress 0x00630498 */
    virtual void PutPixelIndexedNoClip(int nX, int nY, int nIndex);

    /** Slot 17. @ghidraAddress 0x00630508 */
    virtual void PutPixel15NoClip(int nX, int nY, unsigned short nColor);

    /**
     * Slot 21.
     *
     * The program titles this routine after the 32-bit format, and the base's slot 21 is the
     * colourless store rather than a format-specific one, so the title is not the member. Slot 31
     * is the same case on the read side, and both traps appear in ACanvas8 as well.
     *
     * @ghidraAddress 0x00630558
     */
    // The base declares a PutPixelNoClip of its own at a different slot, and declaring this
    // overload would otherwise hide it from lookup on this class. The declaration below affects
    // name lookup only: it adds no slot and changes no layout.
    using ACanvas::PutPixelNoClip;

    virtual void PutPixelNoClip(int nX, int nY, unsigned int nColor);

    /** Slot 23. @ghidraAddress 0x006303f8 */
    virtual void PutPixelNativeNoClip(int nX, int nY, unsigned int nColor);

    /** Slot 25. @ghidraAddress 0x006301b0 */
    virtual int GetPixelIndexedNoClip(int nX, int nY);

    /** Slot 27. @ghidraAddress 0x00630598 */
    virtual unsigned short GetPixel15NoClip(int nX, int nY);

    /** Slot 31. Titled after the 32-bit format; the slot is the colourless read.
     *  @ghidraAddress 0x006305f0 */
    virtual unsigned int GetPixelNoClip(int nX, int nY);

    /** Slot 33. @ghidraAddress 0x00630420 */
    virtual unsigned int GetPixelNativeNoClip(int nX, int nY);

protected:
    // Written as a word by the native setter and as three separate bytes by the index setter, which
    // never touches the fourth. The fourth byte's purpose is unrecovered.
    union {
        unsigned int mColorNative;
        unsigned char mColorChannels[4];
    }; // +0x24
};
