#pragma once

#include "math/color.h"
#include "math/transform.h"
#include "math/vector3.h"
#include "rnd/mat.h"

namespace Rnd {

/**
 * PlayStation 2 material, which drives the GS register state.
 *
 * `Q23Rnd5PsMat` in the RTTI descriptor at `0x008efe50`, with `Rnd::Mat` as its one public base at
 * offset 0 and no member of its own. The sixteen-entry vtable at `0x00830f98` differs from the
 * `Rnd::Mat` table at `0x00822dd8` in six slots, the destructor at slot 1 and the five colour
 * setters at slots 9 through 13.
 *
 * Every override below performs the base assignment and then clears Rnd::g_pSelectedMat, which
 * forces the next draw to re-emit the material registers.
 *
 * The constructor body is empty and the destructor body is one statement. The factory at
 * `0x005914b8` inlines the constructor, and the destructor at `0x00591590` inlines the whole
 * `Rnd::Mat` destructor after its one statement.
 *
 * The hardware binding path is the rest of the file, and every title in it is inferred from the
 * register writes and from the cache global each routine maintains.
 *
 * Which class declares that path is ambiguous. Every call site passes a plain `Rnd::Mat *`,
 * `Rnd::Mesh::DrawInstanced` at `0x005b2f84`, `Rnd::PsParticleSys::Draw` at `0x005fcbcc` and
 * `0x005fcc20`, and `Rnd::PsMesh::DrawSelf` at `0x0060239c` and `0x00602518`, and an upcast to a
 * base at offset 0 emits no instruction, so the image cannot separate a platform-implemented
 * `Rnd::Mat` member from a `Rnd::PsMat` member the caller narrowed to. The declarations are here
 * because the rest of this tree places a platform surface on the `Ps` subclass, as `Rnd::PsTex`
 * does with its GS binding. Moving them to `mat.h` unchanged is the alternative reading.
 */
class PsMat : public Mat {
public:
    /**
     * Construct a material with the default surface.
     *
     * The body is empty. Rnd::NewPsMat() is the only construction site.
     *
     * @param name The object name, passed to the Rnd::Object constructor.
     */
    PsMat(const HxStr &name);

    /** @ghidraAddress 0x00591590 */
    virtual ~PsMat();

    /**
     * Install the PlayStation 2 material creator and build the fixed sphere-map UV transform.
     *
     * GfxDevice::Init() invokes this at `0x0049af48`. The title is inferred from the creator hook
     * the routine overwrites.
     *
     * @ghidraAddress 0x00591240
     */
    static void InstallCreator();

    /** @ghidraAddress 0x005916b0 */
    virtual void SetAmbient(const Color &color);

    /** @ghidraAddress 0x005916c8 */
    virtual void SetDiffuse(const Vector3 &rgb);

    /** @ghidraAddress 0x005916f0 */
    virtual void SetEmissive(const Color &color);

    /** @ghidraAddress 0x00591730 */
    virtual void SetAlpha(float flAlpha);

    /** @ghidraAddress 0x00591708 */
    virtual void SetSpecular(const Vector3 &rgb, float flAlpha);

    /**
     * Apply the next pass of this material to the GS.
     *
     * A material that is already selected and has at most one stage re-emits ALPHA_1 alone, and
     * only when SelectAlphaBlend() overrode it. Otherwise the stage index resets or advances, and
     * the blend mode, the stage texture, and the UV transform are all reselected.
     *
     * @return Non-zero while further stages remain, which asks the caller to draw again.
     * @ghidraAddress 0x0058eb40
     */
    int Select();

    /**
     * Drop the cached material state and program the GS for the default opaque surface.
     *
     * GfxDevice::BeginFrame() invokes this once per frame, and a draw of a mesh with no material
     * invokes it in place of Select().
     *
     * @ghidraAddress 0x005910b0
     */
    static void SelectDefault();

    /**
     * Force ALPHA_1 to the standard source-alpha equation whatever the material blend is.
     *
     * The override is recorded, so the next Select() on the same material restores the material
     * blend. The edge pass of Rnd::PsMesh::DrawSelf() is the only call site.
     *
     * @ghidraAddress 0x00591170
     */
    void SelectAlphaBlend();

private:
    // Program FBA_1, ALPHA_1, DIMX, and TEST_1 for the blend of the selected stage, or for the
    // material blend while stage 0 is selected. 0x0058ec80.
    void SelectBlendMode();

    // Bind the texture of the selected stage, publishing the lighting enable and the texture
    // bound flags the vertex paths read. 0x0058eef8.
    void BindStageTexture();

    // Choose the UV transform the selected stage requires. 0x0058f038.
    void SetupUvXfm();

    // Program CLAMP_1 from a stage wrap mode. The compiler inlined this into BindStageTexture(),
    // and no call site references the out-of-line body at 0x005911d8, so that body is dead in the
    // shipped image.
    void SelectStageClamp(const Stage &stage);
};

/**
 * Allocate and construct a PlayStation 2 material.
 *
 * PsMat::InstallCreator() stores this creator in the material creator hook at `0x00700418`.
 *
 * @param name The object name.
 * @return The new material.
 * @ghidraAddress 0x005914b8
 */
Mat *NewPsMat(const HxStr &name);

/**
 * Coordinate generation mode of the stage Rnd::PsMat::SetupUvXfm() last processed.
 *
 * @ghidraAddress 0x0076d660
 */
extern int g_nSelectedGenMode;

/**
 * Flat shading flag of the selected material.
 *
 * @ghidraAddress 0x0076d664
 */
extern int g_nSelectedFlat;

/**
 * Non-zero once the GS ALPHA_1 register holds a blending equation rather than an opaque copy.
 *
 * @ghidraAddress 0x0076d66c
 */
extern int g_nAlphaBlendEnabled;

/**
 * Non-zero when the bound stage blend is Rnd::Mat::kBlendModeMultiply2.
 *
 * @ghidraAddress 0x0076d670
 */
extern int g_nStageBlendDoubles;

/**
 * Fixed UV transform the selected stage requires, or null when the stage needs none.
 *
 * A sphere generation mode yields the sphere-map transform, and any other mode with a stage
 * transform yields the composed transform.
 *
 * @ghidraAddress 0x0076d674
 */
extern Transform *g_pSelectedUvXfm;

/**
 * Stage transform of the selected stage when it uses one, otherwise null.
 *
 * Only a sphere generation mode publishes a value here, because the other modes fold the stage
 * transform into g_pSelectedUvXfm instead.
 *
 * @ghidraAddress 0x0076d678
 */
extern Transform *g_pSelectedStageXfm;

/**
 * Lighting enable flag of the selected material, cleared when the stage texture function is decal.
 *
 * @ghidraAddress 0x0076d67c
 */
extern int g_nLightingEnabled;

} // namespace Rnd
