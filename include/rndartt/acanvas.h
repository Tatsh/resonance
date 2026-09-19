#pragma once

/**
 * Pixel surface of the art library.
 *
 * `ACanvas` in the RTTI descriptor at `0x0086f660`, a leaf class with no base. The descriptor is
 * the root of a family of nine, `ACanvas8`, `ACanvas15`, `ACanvas24`, and `ACanvas32` for the four
 * pixel depths, and `ACanvasLin4` through `ACanvasLin32` for their linear layouts.
 *
 * Recovery is partial, and only what `Rnd::Font::ComputeCharUV()` touches is settled. That routine
 * takes the surface from `Rnd::Tex::LockMipBitmap()`, reads the two dimensions below as 16-bit
 * values, and probes single pixels through a virtual at vtable slot 32. The class is polymorphic
 * and has no base, so its vptr sits after its data members, and the slot-32 call reads that vptr
 * at `+0x20`; the data therefore occupies `+0x00` through `+0x1f` and the two dimensions are the
 * only members of it that are recovered.
 *
 * The intervening vtable slots belong to the art library and are not recovered, so this
 * declaration does not reproduce the slot index of GetPixel().
 *
 * One conflict is recorded rather than resolved. `Rnd::Tex::mLoadedBitmaps` is a vector of
 * `ABitmap *` and `Rnd::Tex::LockMipBitmap()` yields one of its entries, yet `ABitmap` places an
 * embedded palette at `+0x18` and a palette cannot occupy the offset the vptr is read from. Either
 * the lock yields a canvas rather than the bitmap it was stored as, or the recorded palette offset
 * is wrong. Nothing in `Rnd::Font` separates the two.
 */
class ACanvas {
public:
    virtual ~ACanvas();

    /**
     * Read one pixel.
     *
     * Vtable slot 32. The title is inferred from the single use in the image, which tests the
     * returned value against 0xff000000 to decide whether the pixel is opaque.
     *
     * @param nX The column.
     * @param nY The row.
     * @return The pixel, with its alpha in the high byte.
     */
    virtual unsigned GetPixel(int nX, int nY);

    unsigned char mReserved00[0x06]; /*!< Span of the record that is not recovered. +0x00 */
    short mWidth;                    /*!< Width in pixels. +0x06 */
    short mHeight;                   /*!< Height in pixels. +0x08 */
    unsigned char mReserved0a[0x16]; /*!< Span of the record that is not recovered. +0x0a */
};
