#pragma once

#include <stddef.h>
#include <vector>

#include "math/color.h"
#include "math/transform.h"
#include "math/vector3.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/object.h"

class FailSink;
namespace Rnd {
class Mesh;
class Stream;
class Tex;
} // namespace Rnd

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

        /**
         * Replace the stage's texture.
         *
         * Drops mMat's registration on the previous texture, records the new one, and registers
         * mMat on it. The head-up display's FreQ icon is one caller. The title is inferred.
         *
         * @param pTex The new texture, or null.
         * @ghidraAddress 0x004dd0a0
         */
        void SetTex(Tex *pTex);

        /**
         * Reset the stage to the default surface.
         *
         * The blend becomes kBlendModeMultiply, the coordinate set and the generation mode zero,
         * the transform the identity, the wrap kWrapModeRepeat, and both references null. The
         * routine returns nothing, so it is an initialiser rather than a constructor. The name is
         * inferred.
         *
         * @ghidraAddress 0x004dd020
         */
        void InitDefaults();

        /**
         * Write the stage to the engine text sink.
         *
         * @param sink The text sink.
         * @ghidraAddress 0x004d2270
         */
        void Dump(FailSink &sink) const;

        /**
         * Serialise the stage.
         *
         * Writes the blend, the coordinate set, the generation mode, the twelve transform floats,
         * the transform flag as one byte, the wrap, and the texture as its name. The material
         * reference is not written.
         *
         * @param stream The stream to write to.
         * @ghidraAddress 0x004d26f8
         */
        void Save(Stream &stream) const;

        /**
         * Load the stage.
         *
         * Rnd::g_nRndMatLoadVersion decides the layout. From revision 3 the blend arrives as a
         * BlendMode. Below that it arrives as two words, of which only the first is used, mapped
         * 0, 1, 2, 3, 4, 5, and 6 to kBlendModeDest, kBlendModeSrc, kBlendModeMultiply,
         * kBlendModeAdd, kBlendModeDestAlpha, kBlendModeSrcAlpha, and kBlendModeInvDestAlpha, and
         * any other value leaves the blend as it was. A revision 0 record then names the material
         * the stage belongs to, and every revision ends with the texture.
         *
         * @param stream The stream to read from.
         * @ghidraAddress 0x004d2a20
         */
        void Load(Stream &stream);
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

    /**
     * Allocate a material block under the tag "Rnd::Mat".
     *
     * @param nSize The object size the compiler supplies.
     * @return The block.
     * @ghidraAddress 0x004db900
     */
    static void *operator new(size_t nSize);

    /**
     * Release a material block under the same tag.
     *
     * @param pBlock The block.
     * @ghidraAddress 0x004db920
     */
    static void operator delete(void *pBlock);

    /**
     * Append one stage with the default surface, owned by this material.
     *
     * The routine has no caller in the shipped build. The name is inferred.
     *
     * @ghidraAddress 0x004d2198
     */
    void AddStage();

    /**
     * Remove one stage, dropping this material's reference on its texture.
     *
     * The index is not checked. The routine has no caller in the shipped build. The name is
     * inferred.
     *
     * @param nIndex The position of the stage.
     * @ghidraAddress 0x004dcf60
     */
    void RemoveStage(int nIndex);

    /**
     * Append every referrer of this material whose class is "Mesh" to meshes.
     *
     * Walks the referrer list in order and appends each match through `dynamic_cast`.
     * TnlArena's constructor is the one caller. The definition sits in the TnlArena unit, and the
     * title is inferred.
     *
     * @param meshes The vector to append to.
     * @ghidraAddress 0x00406010
     */
    void GetMeshReferrers(std::vector<Mesh *> &meshes);

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
     * @param nUnknown Ignored by the body. TnlCatcher::SetMultiplied() passes 0 (`0x004552ac`).
     * @ghidraAddress 0x004db958
     */
    virtual void SyncMat(int nUnknown);

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

public:
    /**
     * Colour the surface emits. Defaults to black with full alpha. +0x30
     *
     * Public because HudPoints::SetFrame() at `0x00418de8` copies it directly through a Rnd::Mat
     * pointer from outside the hierarchy to colour a text, and the image has no accessor for it.
     */
    Color mEmissive;

    /**
     * Ambient colour. Defaults to white. +0x40
     *
     * Public because EmitFaceVu1Setup(), SelectLightForVertex(), and the PsMesh upload at
     * `0x00583ba0` read it directly through Rnd::g_pSelectedMat, and the image has no accessor.
     */
    Color mAmbient;
    /**
     * Diffuse colour. Defaults to white. +0x50
     *
     * Public on the same evidence as mAmbient.
     */
    Color mDiffuse;

    // Both edge draw paths read this through a Rnd::Mat pointer from outside the hierarchy, and the
    // image supplies no accessor for it. That is the same evidence that makes mCull public.
    Color mSpecular; // +0x60 Defaults to black with full alpha.

protected:
    int mEnable; // +0x70 Defaults to 1. Serialised as one byte.

public:
    /**
     * Non-zero when the vertex colour supplies the ambient term in place of mAmbient. Serialised as
     * one byte. +0x74
     *
     * Public because EmitFaceVu1Setup(), SelectLightForVertex(), and the PsMesh upload at
     * `0x00583ba0` read it directly through Rnd::g_pSelectedMat, and the image has no accessor.
     */
    int mVertAmbient;
    /**
     * Non-zero when the vertex colour supplies the diffuse term in place of mDiffuse. Serialised as
     * one byte. +0x78
     *
     * Public on the same evidence as mVertAmbient.
     */
    int mVertDiffuse;

protected:
    int mVertSpecular; // +0x7c Serialised as one byte.

public:
    /**
     * Vertex emissive flag. Serialised as one byte. +0x80
     *
     * Public on the same evidence as mVertAmbient.
     */
    int mVertEmissive;
    /**
     * Vertex alpha flag. Serialised as one byte. +0x84
     *
     * Public on the same evidence as mVertAmbient.
     */
    int mVertAlpha;

protected:
    int mNormalize; // +0x88 Serialised as one byte.

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
 * Allocate and construct a material, the base creator of the "Mat" class.
 *
 * @param name The object name.
 * @return The new material.
 * @ghidraAddress 0x004dbd00
 */
Mat *NewMat(const HxStr &name);

/**
 * Build a material through the creator hook.
 *
 * No call site survives in the shipped program. The name is inferred from the Rnd::Button
 * counterpart.
 *
 * @param name The object name.
 * @return The new material.
 * @ghidraAddress 0x004dba28
 */
Mat *NewMatThroughHook(const HxStr &name);

/**
 * Build a material for the registered "Mat" class by calling through g_pfnNewMat.
 *
 * @param name The object name.
 * @return The new material, as its Rnd::Object subobject.
 * @ghidraAddress 0x004dbc80
 */
Object *CreateRegisteredMat(const HxStr &name);

/**
 * Point g_pfnNewMat at NewMat() and register the "Mat" class with Rnd::Manager.
 *
 * The out-of-line copy has no caller, and GfxDevice::Terminate() expands the body. The name is
 * inferred.
 *
 * @ghidraAddress 0x004db9e8
 */
inline void RegisterMatClass() {
    g_pfnNewMat = NewMat;
    g_manager.RegisterClass(g_matClassName, CreateRegisteredMat);
}

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
