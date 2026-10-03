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
class RndAsyncLoader;
namespace Rnd {
class Mat;
class Mesh;
class Object;
class Tex;
} // namespace Rnd

/**
 * Owner of the art and sound assets the FreQ maker works from.
 *
 * Its RTTI descriptor is at `0x0086f7a8`. It is a leaf class with no base. Following the g++ 2.x
 * layout for a class with no base, the vptr sits after the data members at `+0x80`, and the
 * object is 0x84 bytes. The two-entry vtable at `0x007f0908` makes the destructor the one virtual
 * the class declares.
 *
 * The manager loads `MetaGame/persona/freq_maker_inventory_assets.rnd` in the background. Once the
 * load completes, PollLoad() builds one FreqPartTemplate for each part texture it produced, and the
 * two MetFreqLoader objects read the pre-fab personas. The destructor deletes the two loaders and
 * leaves the templates, the asset load, and the two prototypes in place.
 *
 * The translation unit spans `0x0024f798` to `0x00255790`.
 *
 * The one instance is created by CreateInstance(), which allocates exactly 0x84 bytes, runs the
 * constructor, and records the result in the global at `0x006a0f30` that Instance() reads.
 * Creation and access are separate routines, so neither accessor constructs on first use.
 *
 * The public Create(), shared(), and Destroy() are one-call forwarders defined at the start of the
 * unit, and the bodies they reach are defined near its end. Every caller outside the unit goes
 * through a forwarder except MetNullRenderer's constructor, which reads Instance() directly.
 *
 * The titles shared(), PollLoad(), and WaitForLoad() are inferred, and so are the three body
 * titles CreateInstance(), Instance(), and DestroyInstance(). No string in the image identifies
 * any of them.
 */
class MetFreqMakerAssetManager {
public:
    /**
     * Allocate the one instance through CreateInstance().
     *
     * MetRenderer's and MetNullRenderer's constructors are the callers. The title is inferred.
     *
     * @ghidraAddress NTSC-U/C: 0x00254930
     * @ghidraAddress PAL: 0x00269ff8
     */
    static void Create();

    /**
     * Return the one instance through Instance().
     *
     * Fifty call sites across ten classes reach the instance this way.
     *
     * @return The instance, or null before Create().
     * @ghidraAddress NTSC-U/C: 0x00254950
     * @ghidraAddress PAL: 0x0026a018
     */
    static MetFreqMakerAssetManager *shared();

    /**
     * Delete the one instance through DestroyInstance().
     *
     * MetRenderer's and MetNullRenderer's destructors are the callers. The title is inferred.
     *
     * @ghidraAddress NTSC-U/C: 0x00254970
     * @ghidraAddress PAL: 0x0026a038
     */
    static void Destroy();

    /**
     * Allocate the one instance and record it for Instance().
     *
     * Create() is the only caller.
     *
     * @ghidraAddress NTSC-U/C: 0x00255158
     * @ghidraAddress PAL: 0x0026a820
     */
    static void CreateInstance();

    /**
     * Delete the one instance through its virtual destructor.
     *
     * The pointer Instance() reports is not cleared. Destroy() is the only caller.
     *
     * @ghidraAddress NTSC-U/C: 0x002551b8
     * @ghidraAddress PAL: 0x0026a880
     */
    static void DestroyInstance();

    /**
     * Return the one instance, or null before CreateInstance() has created it.
     *
     * shared() and MetNullRenderer's constructor are the callers.
     *
     * @return The instance.
     * @ghidraAddress NTSC-U/C: 0x002551f0
     * @ghidraAddress PAL: 0x0026a8b8
     */
    static MetFreqMakerAssetManager *Instance();

    /**
     * Start with no assets, no loaded templates, and two persona loaders.
     *
     * The loaders read `pers_PS2.dat` and `teamfreq_pers_PS2.dat` under the pre-fab persona
     * directory of GetFreqRoot() into mPrefabIdentities and mTeamFreqIdentities.
     *
     * @ghidraAddress NTSC-U/C: 0x00250b18
     * @ghidraAddress PAL: 0x00266050
     */
    MetFreqMakerAssetManager();

    /**
     * Delete the two persona loaders.
     *
     * The templates, the asset load, and the prototypes are not released.
     *
     * @ghidraAddress NTSC-U/C: 0x00250808
     * @ghidraAddress PAL: 0x00265d40
     */
    virtual ~MetFreqMakerAssetManager();

    /**
     * Advance the asset load by one step and report whether it has finished.
     *
     * A completed load reports success without doing work. Otherwise the routine starts the asset
     * load if needed, pumps the asynchronous loads, and returns false while the asset load is
     * still running. On completion it resolves the prototype material and mesh, rebuilds the part
     * templates from every loaded texture other than the spectrum, the burn prototype, the frame,
     * and the four burn textures, numbers them in load order, marks the load complete, and starts
     * the persona loads.
     *
     * @return True once every asset is resident.
     * @ghidraAddress NTSC-U/C: 0x0024f798
     * @ghidraAddress PAL: 0x00264bd8
     */
    bool PollLoad();

    /**
     * Block until PollLoad() reports the assets resident, running
     * RndAsyncLoader::PollAsyncLoads() between attempts.
     *
     * @ghidraAddress NTSC-U/C: 0x00255200
     * @ghidraAddress PAL: 0x0026a8c8
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
     * @ghidraAddress NTSC-U/C: 0x00255090
     * @ghidraAddress PAL: 0x0026a758
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
     * @ghidraAddress NTSC-U/C: 0x00255100
     * @ghidraAddress PAL: 0x0026a7c8
     */
    std::vector<MetPersonaData *> *GetAllIdentities();

    /**
     * Queue the reads of both persona files.
     *
     * PollLoad() runs it. The title is inferred.
     *
     * @ghidraAddress NTSC-U/C: 0x00254fd0
     * @ghidraAddress PAL: 0x0026a698
     */
    void StartIdentityLoads();

    /**
     * Pump the asynchronous layer and report whether both persona files are parsed.
     *
     * Both loaders are polled on every call. MetSonyScreen's slot 26 calls it, and
     * GetIdentityList() and GetAllIdentities() expand the same body. The title is inferred.
     *
     * @return True once both lists are filled.
     * @ghidraAddress NTSC-U/C: 0x00255000
     * @ghidraAddress PAL: 0x0026a6c8
     */
    bool AreIdentitiesLoaded();

    /**
     * Report through both loaders whether the assets are resident.
     *
     * The image has no caller. The title is inferred.
     *
     * @return True when both loaders report the assets resident.
     * @ghidraAddress NTSC-U/C: 0x00255048
     * @ghidraAddress PAL: 0x0026a710
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
     * @ghidraAddress NTSC-U/C: 0x00254a18
     * @ghidraAddress PAL: 0x0026a0e0
     */
    FreqPartTemplate *GetPart(int nId);

    /**
     * Generate a fresh object name, `autogen_obj_` followed by the value of a counter the call
     * then advances.
     *
     * The title is inferred.
     *
     * @return The name.
     * @ghidraAddress NTSC-U/C: 0x00254e50
     * @ghidraAddress PAL: 0x0026a518
     */
    HxStr NextMeshName();

    /**
     * Create a mesh under one name as a copy of the template mesh, in the default colour.
     *
     * Runs PollLoad() first and discards its result, creates the mesh through Rnd::Mesh::sNew,
     * copies mMeshTemplate into it with no flags, and applies g_freqMakerDefaultColor. The title
     * is inferred.
     *
     * @param name The name of the new mesh.
     * @return The mesh.
     * @ghidraAddress NTSC-U/C: 0x00254a58
     * @ghidraAddress PAL: 0x0026a120
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
     * @ghidraAddress NTSC-U/C: 0x00254b30
     * @ghidraAddress PAL: 0x0026a1f8
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
     * The palette texture, g_spectrumTextureName, is resolved out of Rnd::TheManager on first use
     * and kept in mPaletteTex. The title is inferred.
     *
     * @param position The palette position, each coordinate from 0 to 1.
     * @return The colour, in storage SampleTexture() shares between calls.
     * @ghidraAddress NTSC-U/C: 0x00254f30
     * @ghidraAddress PAL: 0x0026a5f8
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
     * @ghidraAddress NTSC-U/C: 0x00250638
     * @ghidraAddress PAL: 0x00265b70
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
     * @ghidraAddress NTSC-U/C: 0x00254cf8
     * @ghidraAddress PAL: 0x0026a3c0
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
     * @ghidraAddress NTSC-U/C: 0x00254c20
     * @ghidraAddress PAL: 0x0026a2e8
     */
    bool ScaleMesh(
        Rnd::Mesh *pMesh, int nOrthonormalize, float flScaleX, float flScaleY, float flScaleZ);

    /**
     * Report the part templates by name, once PollLoad() has run and its result is discarded.
     *
     * MetFreqMakerInventoryScreen's slot 38 is the one caller. The title is inferred.
     *
     * @return The map.
     * @ghidraAddress NTSC-U/C: 0x00254990
     * @ghidraAddress PAL: 0x0026a058
     */
    std::map<HxStr, FreqPartTemplate *> *GetPartsByName();

    /**
     * Report the part template registered under one name.
     *
     * Runs PollLoad() first and discards its result. The title is inferred.
     *
     * @param name The template name.
     * @return The template, or null when no template has the name.
     * @ghidraAddress NTSC-U/C: 0x002549b8
     * @ghidraAddress PAL: 0x0026a080
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
     * @ghidraAddress NTSC-U/C: 0x00254ea0
     * @ghidraAddress PAL: 0x0026a568
     */
    std::list<FreqPartTemplate *> *TemplatesInCategory(int nCategory);

    /**
     * Create and queue the load of the FreQ maker asset file once.
     *
     * Nothing happens once the load request exists. Otherwise the request for
     * `MetaGame/persona/freq_maker_inventory_assets.rnd` is created with the index of the zone
     * `rndglobal` as its priority and enqueued. MetRenderer's and MetNullRenderer's constructors
     * and PollLoad() call it. The title is inferred.
     *
     * @ghidraAddress NTSC-U/C: 0x0024fce8
     * @ghidraAddress PAL: 0x00265170
     */
    void StartAssetLoad();

    /**
     * Delete every part template and forget them, so the next PollLoad() reloads.
     *
     * Only the name map is emptied. The identifier vector and the category lists retain their
     * pointers. The image has no caller. The title is inferred.
     *
     * @ghidraAddress NTSC-U/C: 0x0024fe58
     * @ghidraAddress PAL: 0x00265320
     */
    void ReleaseParts();

    /**
     * Report a copy of the objects the asset load produced, once PollLoad() has run and its result
     * is discarded.
     *
     * The image has no caller, and PollLoad() expands the same copy. The title is inferred.
     *
     * @return The objects.
     * @ghidraAddress NTSC-U/C: 0x00250540
     * @ghidraAddress PAL: 0x00265a78
     */
    std::list<Rnd::Object *> GetLoadedObjects();

private:
    // NTSC-U/C: 0x0024ff80, PAL: 0x00265448
    // Build a part template over one loaded texture: a material named after it with
    // `.mat` appended, copied from mMaterialTemplate with the texture on its first stage, the
    // texture's bitmap size as its scale, the category the seventh character from the end of the
    // name selects, and the colour and randomisation flags the next two characters set.
    FreqPartTemplate *RegisterPart(Rnd::Object *pObject);

    int mReserved;                                    // +0x00, starts at 0, no reader identified
    MetFreqLoader *mPrefabLoader;                     // +0x04, owned
    MetFreqLoader *mTeamFreqLoader;                   // +0x08, owned
    RndAsyncLoader *mAssetLoader;                     // +0x0c, created by StartAssetLoad()
    int mLoaded;                                      // +0x10, set once PollLoad() completes
    std::map<HxStr, FreqPartTemplate *> mPartsByName; // +0x14
    std::vector<FreqPartTemplate *> mParts;           // +0x20, by template identifier
    Rnd::Mat *mMaterialTemplate;                      // +0x2c, copied by RegisterPart()
    Rnd::Mesh *mMeshTemplate;                         // +0x30, copied by CloneMesh()
    Rnd::Tex *mPaletteTex;                            // +0x34, resolved by ColorAt()
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
 * @ghidraAddress NTSC-U/C: 0x006a0f40
 * @ghidraAddress PAL: 0x006e3220
 */
extern Color g_freqMakerDefaultColor;

/**
 * Registry key of the palette texture MetFreqMakerAssetManager::ColorAt() samples,
 * `spectrum.bmp`.
 *
 * @ghidraAddress NTSC-U/C: 0x006a0f60
 * @ghidraAddress PAL: 0x006e3240
 */
extern HxStr g_spectrumTextureName;
