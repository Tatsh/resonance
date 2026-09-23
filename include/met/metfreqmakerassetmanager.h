#pragma once

#include <list>
#include <map>
#include <vector>

#include "math/color.h"
#include "math/vector2.h"
#include "met/metpersonadata.h"
#include "os/hxstr.h"

class FreqPartTemplate;
class MetFreqLoader;
namespace Rnd {
class Mesh;
class Tex;
} // namespace Rnd

/**
 * Owner of the art and sound assets the FreQ maker works from.
 *
 * `24MetFreqMakerAssetManager` in the RTTI descriptor at `0x0086f7a8`, a leaf class with no base.
 * Following the g++ 2.x layout for a class with no base, the vptr sits after the data members at
 * `+0x80`, so the object is 0x84 bytes. The two-entry vtable is at `0x007f0908`, which makes the
 * destructor the one virtual the class declares.
 *
 * This declaration is deliberately partial. The recovered layout is the shape of the teardown in
 * the destructor at `0x00250808`, which runs in this order.
 *
 *  - Two owned objects at `+0x04` and `+0x08` are released through slot 1 of each object's own
 *    vtable with an `__in_chrg` argument of 3, which is the deleting form.
 *  - A vector of 4-byte elements at `+0x74`, with its finish at `+0x78` and its end of storage at
 *    `+0x7c`, is deallocated.
 *  - A second vector of 4-byte elements at `+0x68` is deallocated.
 *  - The eleven category lists descending from `+0x60` to `+0x38` are cleared through the routine
 *    at `0x00255658`, and each dummy node is returned to the pool.
 *  - The part template vector at `+0x20` is deallocated.
 *
 * The constructor at `0x00250b18` writes the vptr, zeroes `+0x00` through `+0x14`, and then runs
 * on for another 0x400 bytes of asset registration that is not recovered here. The classes of the
 * two owned objects are undetermined. Only the members the recovered routines read are typed, and
 * the rest of the span is recorded as reserved.
 *
 * The one instance is created by Create(), which allocates exactly 0x84 bytes, runs the
 * constructor, and records the result in the global at `0x006a0f30` that shared() reads.
 * Creation and access are separate routines, so shared() does not construct on first use.
 *
 * The titles shared(), PollLoad(), and WaitForLoad() are inferred. No string in the image
 * identifies any of them.
 */
class MetFreqMakerAssetManager {
public:
    /**
     * Return the one instance, or null before Create() has created it.
     *
     * The compiler also emitted an out-of-line copy of this accessor at `0x00254950`.
     *
     * @return The instance.
     * @ghidraAddress 0x002551f0
     */
    static MetFreqMakerAssetManager *shared();

    /**
     * Allocate the one instance and record it for shared().
     *
     * MetRenderer's and MetNullRenderer's constructors reach it through an out-of-line forwarder at
     * `0x00254930`. The title is inferred.
     *
     * @ghidraAddress 0x00255158
     */
    static void Create();

    /**
     * Delete the one instance through its virtual destructor.
     *
     * The pointer shared() reports is not cleared. MetRenderer's and MetNullRenderer's
     * destructors reach it through an out-of-line forwarder at `0x00254970`. The title is
     * inferred.
     *
     * @ghidraAddress 0x002551b8
     */
    static void Destroy();

    /**
     * Release the owned assets, the two vectors, and the list run.
     *
     * @ghidraAddress 0x00250808
     */
    virtual ~MetFreqMakerAssetManager();

    /**
     * Advance the asset load by one step and report whether it has finished.
     *
     * A load already marked finished at `+0x10` reports success without doing work. Otherwise the
     * routine polls the loader at `+0x0c` and returns false while that loader is still running.
     *
     * @return True once every asset is resident.
     * @ghidraAddress 0x0024f798
     */
    bool PollLoad();

    /**
     * Block until PollLoad() reports the assets resident, running
     * RndAsyncLoader::PollAsyncLoads() between attempts.
     *
     * @ghidraAddress 0x00255200
     */
    void WaitForLoad();

    /**
     * Resolve the list of prefabricated identities the FreQ maker offers.
     *
     * The body waits for the load through WaitForLoad() and then spins on AreIdentitiesLoaded()
     * before returning the team FreQ list when GlobalSettings::mTeamFreqUnlocked is set, and the
     * pre-fab list otherwise. The title is inferred from the element type and from the one caller,
     * MetLoadNewFreqScreen::AcquireIdentityList().
     *
     * @return One of the two lists. It is never null.
     * @ghidraAddress 0x00255090
     */
    std::vector<MetPersonaData *> *GetIdentityList();

    /**
     * Resolve the full list of identities, the one at `+0x74`.
     *
     * The body waits for the load through WaitForLoad() and spins on the same two asynchronous
     * requests as GetIdentityList(), and then returns the `+0x74` vector without a selector. The
     * title is inferred from the one caller, CreditsRoll::Reset(), which searches every identity
     * by name.
     *
     * @return The list. It is never null.
     * @ghidraAddress 0x00255100
     */
    std::vector<MetPersonaData *> *GetAllIdentities();

    /**
     * Queue the reads of both persona files.
     *
     * PollLoad() runs it. The title is inferred.
     *
     * @ghidraAddress 0x00254fd0
     */
    void StartIdentityLoads();

    /**
     * Pump the asynchronous layer and report whether both persona files are parsed.
     *
     * Both loaders are polled on every call. MetSonyScreen's slot 26 calls it, and
     * GetIdentityList() and GetAllIdentities() expand the same body. The title is inferred.
     *
     * @return True once both lists are filled.
     * @ghidraAddress 0x00255000
     */
    bool AreIdentitiesLoaded();

    /**
     * Report through both loaders whether the assets are resident.
     *
     * The image has no caller. The title is inferred.
     *
     * @return True when both loaders report the assets resident.
     * @ghidraAddress 0x00255048
     */
    bool AreLoadersReady();

    /**
     * Report the part template with one identifier.
     *
     * Runs PollLoad() first and discards its result. The identifier is not range-checked. The
     * title is inferred.
     *
     * @param nId The template identifier.
     * @return The template.
     * @ghidraAddress 0x00254a18
     */
    FreqPartTemplate *GetPart(int nId);

    /**
     * Generate a fresh object name, `autogen_obj_` followed by the value of a counter the call
     * then advances.
     *
     * The title is inferred.
     *
     * @return The name.
     * @ghidraAddress 0x00254e50
     */
    HxStr NextMeshName();

    /**
     * Create a mesh under one name as a copy of the template mesh, in the default colour.
     *
     * Runs PollLoad() first and discards its result, creates the mesh through Rnd::g_pfnNewMesh,
     * copies mMeshTemplate into it with no flags, and applies g_freqMakerDefaultColor. The title
     * is inferred.
     *
     * @param name The name of the new mesh.
     * @return The mesh.
     * @ghidraAddress 0x00254a58
     */
    Rnd::Mesh *CloneMesh(const HxStr &name);

    /**
     * Scale a mesh to one part template.
     *
     * The template's x and z scales are converted from 1/128 units and reported through pScaleX
     * and pScaleZ. The mesh's local basis is rebuilt as orthonormal around its y row, its x row is
     * scaled by the x scale times flFactorX, its z row by the z scale times flFactorZ, and the mesh
     * is marked dirty. The body does not read this object. The title is inferred.
     *
     * @param pMesh The mesh to scale.
     * @param pTemplate The template whose scale applies.
     * @param pScaleX Receives the template's x scale.
     * @param pScaleZ Receives the template's z scale.
     * @param flFactorX The further factor on the x row.
     * @param flFactorZ The further factor on the z row.
     * @ghidraAddress 0x00254b30
     */
    void ApplyPartScale(Rnd::Mesh *pMesh,
                        FreqPartTemplate *pTemplate,
                        float *pScaleX,
                        float *pScaleZ,
                        float flFactorX,
                        float flFactorZ);

    /**
     * Report the colour at one position of the spectrum palette.
     *
     * The palette texture, g_spectrumTextureName, is resolved out of Rnd::g_manager on first use
     * and kept in mPaletteTex. The title is inferred.
     *
     * @param position The palette position, each coordinate from 0 to 1.
     * @return The colour, in storage SampleTexture() shares between calls.
     * @ghidraAddress 0x00254f30
     */
    Color *ColorAt(const Vector2 &position);

    /**
     * Read the colour of one texel of a texture.
     *
     * The coordinates are scaled by the texture's bitmap size, and the texel's three channels are
     * converted from bytes to the unit range in a Color that every call shares, with an alpha of
     * 1. The body does not read this object. The title is inferred.
     *
     * @param pTex The texture.
     * @param flU The horizontal coordinate, from 0 to 1.
     * @param flV The vertical coordinate, from 0 to 1.
     * @return The shared colour.
     * @ghidraAddress 0x00250638
     */
    Color *SampleTexture(Rnd::Tex *pTex, float flU, float flV);

    /**
     * Write one colour into one texel of a texture.
     *
     * The colour's three channels are scaled to bytes, and the texel under the coordinates scaled
     * by the texture's bitmap size is written. The body does not read this object, and the image
     * has no caller. The title is inferred.
     *
     * @param pTex The texture.
     * @param color The colour. Its alpha is not written.
     * @param flU The horizontal coordinate, from 0 to 1.
     * @param flV The vertical coordinate, from 0 to 1.
     * @return Always true.
     * @ghidraAddress 0x00254cf8
     */
    bool PaintTexel(Rnd::Tex *pTex, const Color &color, float flU, float flV);

    /**
     * Scale the three basis rows of a mesh's local transform.
     *
     * With nOrthonormalize equal to 1 the basis is first rebuilt as orthonormal around its y row.
     * The mesh is marked dirty. The body does not read this object, and the image has no caller.
     * The title is inferred.
     *
     * @param pMesh The mesh.
     * @param nOrthonormalize 1 to rebuild the basis first.
     * @param flScaleX The factor on the x row.
     * @param flScaleY The factor on the y row.
     * @param flScaleZ The factor on the z row.
     * @return Always true.
     * @ghidraAddress 0x00254c20
     */
    bool ScaleMesh(
        Rnd::Mesh *pMesh, int nOrthonormalize, float flScaleX, float flScaleY, float flScaleZ);

    /**
     * Report the part templates by name, once PollLoad() has run and its result is discarded.
     *
     * MetFreqMakerInventoryScreen's slot 38 is the one caller. The title is inferred.
     *
     * @return The map.
     * @ghidraAddress 0x00254990
     */
    std::map<HxStr, FreqPartTemplate *> *GetPartsByName();

    /**
     * Report the part template registered under one name.
     *
     * Runs PollLoad() first and discards its result. The title is inferred.
     *
     * @param name The template name.
     * @return The template, or null when no template has the name.
     * @ghidraAddress 0x002549b8
     */
    FreqPartTemplate *FindPart(const HxStr &name);

    /** The number of part categories, which count from 1. */
    static constexpr int kCategoryCount = 11;

    /**
     * Report the templates of one category.
     *
     * A category outside 1 through 11 reports the list of category 11. The title is inferred.
     *
     * @param nCategory The category.
     * @return The list.
     * @ghidraAddress 0x00254ea0
     */
    std::list<FreqPartTemplate *> *TemplatesInCategory(int nCategory);

private:
    // The members the destructor walks, described in the class documentation above.
    unsigned char mUnknown00[0x4];                                // +0x00
    MetFreqLoader *mPrefabLoader;                                 // +0x04, owned
    MetFreqLoader *mTeamFreqLoader;                               // +0x08, owned
    unsigned char mUnknown0c[0x8];                                // +0x0c
    std::map<HxStr, FreqPartTemplate *> mPartsByName;             // +0x14
    std::vector<FreqPartTemplate *> mParts;                       // +0x20, by template identifier
    unsigned char mUnknown2c[0x4];                                // +0x2c
    Rnd::Mesh *mMeshTemplate;                                     // +0x30, copied by CloneMesh()
    Rnd::Tex *mPaletteTex;                                        // +0x34, resolved by ColorAt()
    std::list<FreqPartTemplate *> mCategoryLists[kCategoryCount]; // +0x38, categories 1 to 11
    // Advanced by NextMeshName().
    int mMeshCount;                                    // +0x64
    std::vector<MetPersonaData *> mPrefabIdentities;   // +0x68, filled by mPrefabLoader
    std::vector<MetPersonaData *> mTeamFreqIdentities; // +0x74, filled by mTeamFreqLoader
};

/**
 * Colour a freshly cloned FreQ maker mesh and a reset FreqPart start with, an opaque grey of
 * 0.75 in each channel.
 *
 * @ghidraAddress 0x006a0f40
 */
extern Color g_freqMakerDefaultColor;

/**
 * Registry key of the palette texture MetFreqMakerAssetManager::ColorAt() samples,
 * `spectrum.bmp`.
 *
 * @ghidraAddress 0x006a0f60
 */
extern HxStr g_spectrumTextureName;
