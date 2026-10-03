#pragma once

#include "math/vector3.h"
#include "rnd/mat.h"

struct Color;
struct Transform;

namespace Rnd {

/**
 * PlayStation 2 material, which drives the GS register state.
 *
 * Its RTTI descriptor is at `0x008efe50`. It has `Rnd::Mat` as its one public base at offset 0 and
 * no member. The sixteen-entry vtable at `0x00830f98` differs from the `Rnd::Mat` table
 * at `0x00822dd8` in six slots, the destructor at slot 1 and the five colour setters at slots 9
 * through 13.
 *
 * Every override below performs the base assignment and then clears sCurrent, which
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
 * `0x005fcc20`, and `Rnd::PsMesh::DrawShowing` at `0x0060239c` and `0x00602518`, and an upcast to a
 * base at offset 0 emits no instruction, so the image cannot separate a platform-implemented
 * `Rnd::Mat` member from a `Rnd::PsMat` member the caller narrowed to. The declarations are here
 * because the rest of this tree places a platform surface on the `Ps` subclass, as `Rnd::PsTex`
 * does with its GS binding. Moving them to `mat.h` unchanged is the alternative reading.
 */
class PsMat : public Mat {
public:
    /**
     * Material that Rnd::Mat::SelectMaterial() last applied, or null when the applied state is
     * stale.
     *
     * Selecting a material stores it here, and the PlayStation 2 entry point compares against it
     * to skip work that is already done. Every property setter writes null to make the next
     * selection apply the change, and Rnd::PsMat's destructor clears it when the material being
     * destroyed is the selected one.
     *
     * @ghidraAddress NTSC-U/C: 0x0076d658
     * @ghidraAddress PAL: 0x007b13b0
     */
    static Mat *sCurrent;

    /**
     * Construct a material with the default surface.
     *
     * The body is empty. Rnd::NewPsMat() is the only construction site.
     *
     * @param name The object name, passed to the Rnd::Object constructor.
     */
    PsMat(const HxStr &name);

    /**
     * @ghidraAddress NTSC-U/C: 0x00591590
     * @ghidraAddress PAL: 0x005d4928
     */
    virtual ~PsMat();

    /**
     * Install the PlayStation 2 material creator and build the fixed sphere-map UV transform.
     *
     * GfxDevice::Init() invokes this at `0x0049af48`. The title is inferred from the creator hook
     * the routine overwrites.
     *
     * @ghidraAddress NTSC-U/C: 0x00591240
     * @ghidraAddress PAL: 0x005d45c0
     */
    static void InstallCreator();

    /**
     * @ghidraAddress NTSC-U/C: 0x005916b0
     * @ghidraAddress PAL: 0x005d4a48
     */
    virtual void SetAmbient(const Color &color);

    /**
     * @ghidraAddress NTSC-U/C: 0x005916c8
     * @ghidraAddress PAL: 0x005d4a60
     */
    virtual void SetDiffuse(const Color &color);

    /**
     * @ghidraAddress NTSC-U/C: 0x005916f0
     * @ghidraAddress PAL: 0x005d4a88
     */
    virtual void SetEmissive(const Color &color);

    /**
     * @ghidraAddress NTSC-U/C: 0x00591730
     * @ghidraAddress PAL: 0x005d4ac8
     */
    virtual void SetAlpha(float flAlpha);

    /**
     * @ghidraAddress NTSC-U/C: 0x00591708
     * @ghidraAddress PAL: 0x005d4aa0
     */
    virtual void SetSpecular(const Color &color, float flAlpha);

    /**
     * Apply the next pass of this material to the GS.
     *
     * A material that is already selected and has at most one stage re-emits ALPHA_1 alone, and
     * only when SelectAlphaBlend() overrode it. Otherwise the stage index resets or advances, and
     * the blend mode, the stage texture, and the UV transform are all reselected.
     *
     * @return Non-zero while further stages remain, which asks the caller to draw again.
     * @ghidraAddress NTSC-U/C: 0x0058eb40
     * @ghidraAddress PAL: 0x005d1e98
     */
    int Select();

    /**
     * Drop the cached material state and program the GS for the default opaque surface.
     *
     * GfxDevice::BeginFrame() invokes this once per frame, and a draw of a mesh with no material
     * invokes it in place of Select().
     *
     * @ghidraAddress NTSC-U/C: 0x005910b0
     * @ghidraAddress PAL: 0x005d4430
     */
    static void SelectDefault();

    /**
     * Force ALPHA_1 to the standard source-alpha equation whatever the material blend is.
     *
     * The override is recorded, so the next Select() on the same material restores the material
     * blend. The edge pass of Rnd::PsMesh::DrawShowing() is the only call site.
     *
     * @ghidraAddress NTSC-U/C: 0x00591170
     * @ghidraAddress PAL: 0x005d44f0
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
 * @ghidraAddress NTSC-U/C: 0x005914b8
 * @ghidraAddress PAL: 0x005d4850
 */
Mat *NewPsMat(const HxStr &name);

/**
 * Coordinate generation mode of the stage Rnd::PsMat::SetupUvXfm() last processed.
 *
 * @ghidraAddress NTSC-U/C: 0x0076d660
 * @ghidraAddress PAL: 0x007b13b8
 */
extern int g_nSelectedGenMode;

/**
 * Flat shading flag of the selected material.
 *
 * @ghidraAddress NTSC-U/C: 0x0076d664
 * @ghidraAddress PAL: 0x007b13bc
 */
extern int g_nSelectedFlat;

/**
 * Non-zero once the GS ALPHA_1 register holds a blending equation rather than an opaque copy.
 *
 * @ghidraAddress NTSC-U/C: 0x0076d66c
 * @ghidraAddress PAL: 0x007b13c4
 */
extern int g_nAlphaBlendEnabled;

/**
 * Non-zero when the bound stage blend is Rnd::Mat::kBlendModeMultiply2.
 *
 * @ghidraAddress NTSC-U/C: 0x0076d670
 * @ghidraAddress PAL: 0x007b13c8
 */
extern int g_nStageBlendDoubles;

/**
 * Fixed UV transform the selected stage requires, or null when the stage needs none.
 *
 * A sphere generation mode yields the sphere-map transform, and any other mode with a stage
 * transform yields the composed transform.
 *
 * @ghidraAddress NTSC-U/C: 0x0076d674
 * @ghidraAddress PAL: 0x007b13cc
 */
extern Transform *g_pSelectedUvXfm;

/**
 * Stage transform of the selected stage when it uses one, otherwise null.
 *
 * Only a sphere generation mode publishes a value here, because the other modes fold the stage
 * transform into g_pSelectedUvXfm instead.
 *
 * @ghidraAddress NTSC-U/C: 0x0076d678
 * @ghidraAddress PAL: 0x007b13d0
 */
extern Transform *g_pSelectedStageXfm;

/**
 * Lighting enable flag of the selected material, cleared when the stage texture function is decal.
 *
 * @ghidraAddress NTSC-U/C: 0x0076d67c
 * @ghidraAddress PAL: 0x007b13d4
 */
extern int g_nLightingEnabled;

} // namespace Rnd
