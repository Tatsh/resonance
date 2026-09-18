#pragma once

#include "math/color.h"
#include "math/vector3.h"
#include "rnd/mat.h"

namespace Rnd {

/**
 * PlayStation 2 material, which drives the GS register state.
 *
 * `Q23Rnd5PsMat` in the RTTI descriptor at `0x008efe50`, with `Rnd::Mat` as its one public base at
 * offset 0 and no member of its own.
 *
 * Every override below performs the base assignment and then clears the cached selection global at
 * `0x0076d658`, which forces the next draw to re-emit the material registers.
 *
 * Recovery is partial. The binding routines in the same file are the material selection at
 * `0x0058eb40` and `0x00591170`, the blend mode selection at `0x0058ec80`, the stage texture bind
 * at `0x0058eef8`, the texture coordinate transform setup at `0x0058f038`, the default material
 * selection at `0x005910b0`, the tagged allocation at `0x005914b8`, and the class registration at
 * `0x00591240`. None of their bodies is reconstructed, and their receivers are not yet confirmed.
 */
class PsMat : public Mat {
public:
    /**
     * Construct a material with the default surface.
     *
     * @param name The object name, passed to the Rnd::Object constructor.
     */
    PsMat(const HxStr &name);

    /** @ghidraAddress 0x00591590 */
    virtual ~PsMat();

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
};

} // namespace Rnd
