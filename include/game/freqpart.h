#pragma once

#include "math/color.h"
#include "math/vector2.h"
#include "math/vector3.h"

class FreqPartTemplate;
namespace Rnd {
class Mesh;
}

/**
 * One placed part of a FreQ avatar.
 *
 * The class is not polymorphic and emits no RTTI descriptor, and no literal in the image
 * identifies it, so the name is inferred from its one owner. FreqAppearanceDetail stores a
 * std::list of pointers to these, and each part pairs a FreQ maker part template with a placement,
 * a palette position, a mesh cloned for it, and the colour that mesh is drawn in. The destructor
 * takes an `__in_chrg` argument and the allocation that FreqAppearanceDetail makes is 0x40 bytes.
 *
 * The translation unit spans `0x00174de8` to `0x001773c0`. Besides the members below, it has
 * template library emissions at `0x00174f28`, `0x00175100`, `0x001761f8`, `0x00176790`,
 * `0x00176bb8`, `0x00176cc0`, `0x00176dc8`, and `0x001772f8`.
 */
class FreqPart {
public:
    /**
     * Eight-byte transfer form of a part that Pack() writes and Unpack() reads.
     *
     * The title is inferred.
     */
    struct Packed {
        short mId;               /*!< The template identifier, negated when mirrored. */
        short mX;                /*!< The placement x, truncated. */
        short mZ;                /*!< The placement z, truncated. */
        unsigned char mPaletteX; /*!< The palette x scaled to 0 through 255. */
        unsigned char mPaletteY; /*!< The palette y scaled to 0 through 255. */
    };

    /**
     * Construct a part with no template, run through Reset().
     *
     * @ghidraAddress 0x00176f38
     */
    FreqPart();

    /**
     * Construct a part of one template at the origin.
     *
     * @param pTemplate The template the part is an instance of.
     * @ghidraAddress 0x00176ed0
     */
    explicit FreqPart(FreqPartTemplate *pTemplate);

    /**
     * Copy another part through operator=().
     *
     * @param other The part to copy.
     * @ghidraAddress 0x00176f78
     */
    FreqPart(const FreqPart &other);

    /**
     * Delete the mesh.
     *
     * @ghidraAddress 0x00176fd0
     */
    ~FreqPart();

    /**
     * Copy another part, cloning a fresh mesh from the other part's mesh.
     *
     * The template, the placement, the mirrored flag, and the colour are copied first. The mesh
     * this part has is deleted, a new one is cloned through the FreQ maker asset manager under a
     * generated name, the other part's mesh is copied into it, the colour is applied to it, and
     * the palette position is copied last. The routine returns nothing.
     *
     * @param other The part to copy.
     * @ghidraAddress 0x00174de8
     */
    void operator=(const FreqPart &other);

    /**
     * Report the mesh.
     *
     * Defined in the header. The out-of-line copy is called by FreqAppearanceDetail.
     *
     * @return The mesh, or null.
     * @ghidraAddress 0x001770f8
     */
    Rnd::Mesh *GetMesh() const {
        return mMesh;
    }

    /**
     * Replace the mesh, deleting the one this part had.
     *
     * @param pMesh The new mesh.
     * @ghidraAddress 0x00177100
     */
    void SetMesh(Rnd::Mesh *pMesh);

    /**
     * Report the colour the mesh is drawn in.
     *
     * Defined in the header. The out-of-line copy is called by FreqAppearance::RenderBurnTextures()
     * and FreqAppearanceDetail.
     *
     * @return The colour.
     * @ghidraAddress 0x00177158
     */
    Color *GetColor() {
        return &mColor;
    }

    /**
     * Record a colour and apply it to the mesh.
     *
     * A part whose template does not take a colour records opaque black in its place.
     *
     * @param color The colour.
     * @ghidraAddress 0x00177160
     */
    void SetColor(const Color &color);

    /**
     * Clear every member without deleting the mesh.
     *
     * The palette position becomes (-1, -1) and the colour becomes g_freqMakerDefaultColor.
     *
     * @ghidraAddress 0x001771b8
     */
    void Reset();

    /**
     * Write the transfer form.
     *
     * The palette position is clamped to the unit range, stored back, and scaled to a byte.
     *
     * @param pOut The record to write.
     * @ghidraAddress 0x001771f8
     */
    void Pack(Packed *pOut);

    /**
     * Read the transfer form.
     *
     * The template is looked up through the FreQ maker asset manager by the absolute identifier,
     * and a negative identifier marks the part mirrored. The placement y is cleared.
     *
     * @param packed The record to read.
     * @ghidraAddress 0x00177038
     */
    void Unpack(const Packed &packed);

private:
    FreqPartTemplate *mTemplate; // +0x00
    // Not written by any recovered routine.
    unsigned char mUnknown04[0xc]; // +0x04
    Vector3 mPosition;             // +0x10
    int mMirrored;                 // +0x20
    Vector2 mPalettePosition;      // +0x24
    Rnd::Mesh *mMesh;              // +0x2c
    Color mColor;                  // +0x30
};
