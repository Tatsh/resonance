#pragma once

#include <vector>

#include "math/color.h"
#include "math/transform.h"
#include "math/vector3.h"
#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/object.h"
#include "rnd/stream.h"
#include "rnd/tex.h"

namespace Rnd {

/**
 * Surface description a Rnd::Mesh draws with.
 *
 * `Q23Rnd3Mat` in the RTTI descriptor at `0x008ef0e0`, with `Rnd::Object` as its one public
 * non-virtual base at offset 0. The Object subobject is 0x1c bytes, so the material's own members
 * start at `+0x1c` and the whole object is 0xa0 bytes.
 *
 * A material combines the four lighting colours, the flags that pick per-vertex colours over the
 * material colours, and a vector of texture stages. The stage vector is what binds textures; the
 * material itself has no texture reference.
 *
 * The PlayStation 2 subclass Rnd::PsMat adds the hardware binding path and overrides the one
 * material virtual to invalidate the cached register state.
 */
class Mat : public Object {
public:
    /**
     * Frame buffer blend, as the text dump titles the values.
     *
     * The same enumeration types a texture stage, where the default is kBlendModeMultiply.
     */
    enum BlendMode {
        kBlendModeDest = 0,
        kBlendModeSrc = 1,
        kBlendModeAdd = 2,
        kBlendModeMultiply = 3,
        kBlendModeMultiply2 = 4,
        kBlendModeSrcAlpha = 5,
        kBlendModeSrcAlphaAdd = 6,
        kBlendModeSrcAdd = 7,
        kBlendModeInvSrcAlpha = 8,
        kBlendModeDestAlpha = 9,
        kBlendModeInvDestAlpha = 10,
        kBlendModeSrcAlphaOpaque = 11,
        kBlendModeSrcAlphaCutout = 12
    };

    /** Face winding the rasteriser discards, as the text dump titles the values. */
    enum CullMode {
        kCullModeCw = 0,  /*!< Discard clockwise faces. */
        kCullModeCcw = 1, /*!< Discard counter-clockwise faces. */
        kCullModeNone = 2 /*!< Draw both windings. */
    };

    /**
     * One texture stage of a material.
     *
     * The structure is 0x60 bytes and emits no RTTI descriptor. Its member names come from the
     * labels the stage text dump writes, "\n\tblend:", " coordIndex:", " genMode:", " xfm:",
     * "useXfm:", " wrap:", " mat:", and " tex:". The dump writes the material reference before the
     * texture reference even though the texture is stored first.
     */
    struct Stage {
        /** How the stage produces its texture coordinates. */
        enum GenMode {
            kGenModeFixed = 0,     /*!< Use the vertex coordinates as they are. */
            kGenModeSphere = 1,    /*!< Sphere map. */
            kGenModePlanar = 2,    /*!< Planar projection. */
            kGenModeOrthoCube = 3, /*!< Cube map in object space. */
            kGenModeLocalCube = 4  /*!< Cube map in local space. */
        };

        /** How a coordinate outside the unit square resolves. */
        enum WrapMode { kWrapModeClamp = 0, kWrapModeRepeat = 1, kWrapModeMirror = 2 };

        BlendMode mBlend; // +0x00 Defaults to kBlendModeMultiply.
        int mCoordIndex;  // +0x04 Which of the two vertex texture coordinate sets to read.
        GenMode mGenMode; // +0x08
        Transform mXfm;   // +0x10 Identity until a file or an animation writes it.
        int mUseXfm;      // +0x50
        WrapMode mWrap;   // +0x54 Defaults to kWrapModeRepeat.
        Tex *mTex;        // +0x58
        Mat *mMat;        // +0x5c
    };

    /** Serial version this build writes, and the highest version it loads. */
    enum { kSerialVersion = 7 };

    /**
     * Construct a material with the default surface.
     *
     * Ambient and diffuse start white, emissive and specular start black with full alpha, the
     * blend starts at kBlendModeSrcAlpha, and lighting starts enabled.
     *
     * @param name The object name, passed to the Rnd::Object constructor.
     * @ghidraAddress 0x004d0ea8
     */
    Mat(const HxStr &name);

    /** @ghidraAddress 0x004dbb10 */
    virtual ~Mat();

protected:
    // Drop this material's reference on every stage texture. Walks mStages and calls
    // Rnd::Object::RemoveRef() for each stage whose mTex is set. Both destructors in the hierarchy
    // call it out of line, this class's at 0x004dbb10 and Rnd::PsMat's at 0x005915d4, which is the
    // evidence for protected rather than private access. Copy() and Load() inline the same body
    // instead of calling it. 0x004dcd20.
    void RemoveStageTexRefs();

public:
    /**
     * Write the material to the engine text sink.
     *
     * @param sink The text sink.
     * @ghidraAddress 0x004d0f78
     */
    virtual void DumpText(FailSink &sink);

    /**
     * Serialise the material.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x004d1638
     */
    virtual void Save(Stream &stream);

    /**
     * Replace one object reference with another.
     *
     * A material references objects only through its stages, so the routine walks the stage vector
     * and swaps every texture reference whose current value is the old object.
     *
     * @param pFrom The object being replaced.
     * @param pTo The object to point at, which may be null.
     * @ghidraAddress 0x004dcc20
     */
    virtual void Replace(Object *pFrom, Object *pTo);

    /**
     * Return the registered class name, "Mat".
     *
     * The key is the one a data file writes for the class, which the registry at
     * `Rnd::Manager::Init()` maps to the creator at 0x004dbc80.
     *
     * @return The class name.
     * @ghidraAddress 0x004dbc70
     */
    virtual const HxStr &ClassName() const;

    /**
     * Copy another material over this one.
     *
     * @param pSource The source object, which has to be a material for the copy to have any
     *                effect.
     * @param nFlags The copy flags.
     * @ghidraAddress 0x004dce18
     */
    virtual void Copy(const Object *pSource, unsigned nFlags);

    /**
     * Load the material.
     *
     * Reports "Can't load new Mat" through the failure sink when the file version exceeds
     * kSerialVersion. A file below version 3 stores a source factor and a destination factor
     * instead of a blend mode, and the loader folds the pair into the nearest blend mode.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x004d1a90
     */
    virtual void Load(Stream &stream);

    /**
     * Virtual with an empty body in Rnd::Mat.
     *
     * No subclass in the shipped build overrides it, so its purpose is undetermined. It fills the
     * ninth vtable slot.
     *
     * @ghidraAddress 0x004db958
     */
    virtual void SyncMat();

    /**
     * Set the ambient colour.
     *
     * Rnd::PsMat overrides the setter to invalidate the cached hardware material state.
     *
     * @param color The new ambient colour.
     * @ghidraAddress 0x004db970
     */
    virtual void SetAmbient(const Color &color);

    /**
     * Set the diffuse colour, retaining the current alpha.
     *
     * The body reads three floats and the fourth component of the argument is discarded, so the
     * parameter type is not recoverable from the body alone. The one caller fixes it.
     * Rnd::MatAnim::SetFrameSelf() blends the diffuse channel over all four components with
     * `vmulax.xyzw` at `0x004d5028`, stores the whole quadword into the temporary it passes, and
     * blends the three stage channels of the same routine over three components with `vmulax.xyz`
     * instead. The argument is therefore a Color and not a Vector3.
     *
     * @param color The new diffuse colour, whose alpha is discarded.
     * @ghidraAddress 0x004db980
     */
    virtual void SetDiffuse(const Color &color);

    /**
     * Set the emissive colour.
     *
     * @param color The new emissive colour.
     * @ghidraAddress 0x004db9a0
     */
    virtual void SetEmissive(const Color &color);

    /**
     * Set the diffuse alpha.
     *
     * @param flAlpha The new alpha.
     * @ghidraAddress 0x004db9b0
     */
    virtual void SetAlpha(float flAlpha);

    /**
     * Set the specular colour and its alpha.
     *
     * The colour argument is a Color on the same evidence as SetDiffuse(), the specular blend at
     * `0x004d54d8` being the fourth of the four quadword blends. The alpha of the colour is
     * discarded and the separate argument is stored in its place, and the only caller passes zero
     * for it.
     *
     * @param color The new specular colour, whose alpha is discarded.
     * @param flAlpha The new specular alpha.
     * @ghidraAddress 0x004db9b8
     */
    virtual void SetSpecular(const Color &color, float flAlpha);

    /**
     * Set the lighting enable flag, the five per-vertex source flags, and the normalise flag.
     *
     * @param nEnable Whether lighting applies at all.
     * @param nVertAmbient Whether the ambient term comes from the vertex.
     * @param nVertDiffuse Whether the diffuse term comes from the vertex.
     * @param nVertSpecular Whether the specular term comes from the vertex.
     * @param nVertEmissive Whether the emissive term comes from the vertex.
     * @param nVertAlpha Whether the alpha comes from the vertex.
     * @param nNormalize Whether normals are renormalised.
     * @ghidraAddress 0x004dcbc0
     */
    virtual void SetLighting(int nEnable,
                             int nVertAmbient,
                             int nVertDiffuse,
                             int nVertSpecular,
                             int nVertEmissive,
                             int nVertAlpha,
                             int nNormalize);

    /**
     * Resynchronise the material after its state or its stage vector has changed.
     *
     * Copy() finishes with this call. The name is inferred from the position of the call and
     * from the stage vector the routine walks.
     *
     * @ghidraAddress 0x004dcd88
     */
    virtual void Refresh();

    /**
     * Set the multi-pass count.
     *
     * @param nMultiPass The new count.
     * @ghidraAddress 0x004db960
     */
    void SetMultiPass(int nMultiPass);

    /**
     * Set whether the surface shades flat.
     *
     * @param nFlat Non-zero for flat shading.
     * @ghidraAddress 0x004db968
     */
    void SetFlat(int nFlat);

    // Rnd::Font::ComputeCharUV at 0x004ca050 reads the vector's bounds through a Rnd::Mat pointer
    // from outside the hierarchy, divides the span by 96 to size it, and then reads the first
    // stage's texture. The image supplies no accessor. That is the same evidence that makes
    // mSpecular public. The assignment operator Copy() uses is instantiated out of line at
    // 0x004d77d8, which is library code and has no body in this tree.
    std::vector<Stage> mStages; // +0x1c

protected:
    // Every member below is protected rather than private. Rnd::PsMat writes four of the colours in
    // its setter overrides, and the material selection path in the same file reads the whole
    // surface to build the GS register writes. The order below is the recovered offset order.
    BlendMode mBlend; // +0x28 Defaults to kBlendModeSrcAlpha.
    Color mEmissive;  // +0x30 Defaults to black with full alpha.
    Color mAmbient;   // +0x40 Defaults to white.
    Color mDiffuse;   // +0x50 Defaults to white.

public:
    // Both edge draw paths read this through a Rnd::Mat pointer from outside the hierarchy, and the
    // image supplies no accessor for it. That is the same evidence that makes mCull public.
    Color mSpecular; // +0x60 Defaults to black with full alpha.

protected:
    int mEnable;       // +0x70 Defaults to 1. Serialised as one byte.
    int mVertAmbient;  // +0x74 Serialised as one byte.
    int mVertDiffuse;  // +0x78 Serialised as one byte.
    int mVertSpecular; // +0x7c Serialised as one byte.
    int mVertEmissive; // +0x80 Serialised as one byte.
    int mVertAlpha;    // +0x84 Serialised as one byte.
    int mNormalize;    // +0x88 Serialised as one byte.

public:
    /*!< Winding the rasteriser discards. Public because Rnd::Mesh::Collide() at 0x0047f950 reads
         it directly to decide whether a back-facing hit counts, which is access from outside the
         hierarchy, and the image has no accessor for it. +0x8c */
    CullMode mCull;

protected:
    int mMultiPass; // +0x90
    int mFlat;      // +0x94 Serialised as one byte.
};

/**
 * Registered class name of Rnd::Mat, the string "Mat".
 *
 * @ghidraAddress 0x00700420
 */
extern HxStr g_matClassName;

/**
 * Serial version of the material record currently being read.
 *
 * Load() reads the version into this global at its start and then tests it, the same arrangement
 * Rnd::Mesh uses with its own word at `0x00894d68`. The two are distinct globals, so a material
 * loaded inside a mesh record does not disturb the mesh version.
 *
 * @ghidraAddress 0x00894e2c
 */
extern int g_nRndMatLoadVersion;

/**
 * Creator the registered "Mat" class builds through.
 *
 * Rnd::PsMat::InstallCreator() overwrites the hook with the Rnd::PsMat creator at `0x005914b8`,
 * the same arrangement Rnd::Tex uses.
 *
 * @ghidraAddress 0x00700418
 */
extern Mat *(*g_pfnNewMat)(const HxStr &name);

/**
 * Material that Rnd::Mat::SelectMaterial() last applied, or null when the applied state is stale.
 *
 * Selecting a material stores it here, and the PlayStation 2 entry point compares against it to
 * skip work that is already done. Every property setter writes null so that the next selection
 * applies the change, and Rnd::PsMat's destructor clears it when the dying material is the
 * selected one.
 *
 * @ghidraAddress 0x0076d658
 */
extern Mat *g_pSelectedMat;

/**
 * Non-zero while the material stage being applied has a texture bound.
 *
 * Rnd::Mat::BindStageTexture() sets it from whether the bind succeeded. Rnd::PsMesh reads it to
 * choose how much of each vertex to upload, which is what identifies Rnd::MeshVert's fourth
 * quadword as the texture coordinate.
 *
 * @ghidraAddress 0x0076d668
 */
extern int g_nStageTextureBound;

} // namespace Rnd
