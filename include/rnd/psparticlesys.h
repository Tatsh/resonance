#pragma once

#include "rnd/particlesys.h"

class HxStr;

namespace Rnd {

/**
 * PlayStation 2 particle system, which submits its particles to the GS.
 *
 * `Q23Rnd13PsParticleSys` in the RTTI descriptor at `0x008f1250`, with `Rnd::ParticleSys` as its
 * one public base at offset 0 and no member of its own. The factory allocates 0x220 bytes, the
 * same as the base, which is what shows the subclass adds no data.
 *
 * The subclass overrides exactly two virtuals, the destructor in the Object subobject table at
 * `0x0083a250` and DrawSelf() in the Drawable table at `0x0083a298`. Every other slot of all four
 * tables still addresses the Rnd::ParticleSys implementation. The constructor body is empty.
 *
 * Three submission paths serve the three modes, and which one runs also depends on whether the
 * device is on the vector unit path. With VU1 selected only a sprite system draws at all, through
 * DrawSpritesDmaKicked(); the point and line modes submit nothing. Without it the particles are
 * first packed into the shared draw buffer by PackParticleQuads(), two Rnd::DrawVert records per
 * particle, and then one of three emitters runs.
 *
 * All three emitters share the buffer Rnd::PsMesh uses, which is why `g_aDrawVerts` is declared
 * shared rather than owned by the mesh path, and all three respect the material state
 * Rnd::PsMat::Select() published. The sprite emitter reads g_nStageTextureBound to choose between
 * a six-register and a three-register GIFtag, and its PRIM word takes TME from the same global and
 * ABE from g_nAlphaBlendEnabled.
 *
 * The overflow guard mirrors the mesh one exactly. A population above 1250 reports "DrawShowing
 * particle buffer overflow... %d" and continues rather than clamping, which is faithful.
 */
class PsParticleSys : public ParticleSys {
public:
    /**
     * Construct an empty PlayStation 2 particle system.
     *
     * The body is empty. Rnd::NewPsParticleSys() is the only construction site.
     *
     * @param name The object name, passed to the Rnd::Object constructor.
     * @ghidraAddress 0x005fcdf0
     */
    PsParticleSys(const HxStr &name);

    /** @ghidraAddress 0x005ff878 */
    virtual ~PsParticleSys();

protected:
    /**
     * Draw the particles.
     *
     * Advances the mesh draw counter, returns at once when no particle is live, programs
     * ZBUF_1.ZMSK to suppress depth writes and TEST_1.ZTST from mReadZ, and selects the material.
     * The vector unit path then submits sprites alone, repeating the whole submission once per
     * material pass. The software path packs the particles first and dispatches on the mode.
     *
     * @return Always non-zero, so the children always draw.
     * @ghidraAddress 0x005fcb18
     */
    virtual int DrawSelf();

private:
    // Submit one GS line per particle from the packed vertex pairs. Each pair yields a two-vertex
    // line with a colour of its own, and the line counter advances by the whole vertex count
    // before any of them is examined. 0x005fc570.
    void EmitGifLines(int nVertCount);

    // Submit one GS sprite per particle from the packed vertex pairs. The first vertex of a pair
    // is the near corner and the second the far one, and a pair is rejected when the near corner
    // falls below zero or the far corner passes 0xffff in either axis. A textured sprite sends
    // three quadwords per vertex, and an untextured one sends the colour and both positions.
    // 0x005fc6d0.
    void EmitGifSprites(int nVertCount);

    // Upload the live list to VU1 as sprite records and call the microprogram. Two quadwords
    // travel per particle, the colour and the position, and half the size is written into the
    // fourth word of the position first because a GS sprite takes a centre and a half extent.
    // A batch closes at 162 particles or at 254 destination quadwords, whichever comes first, and
    // the first batch enters through MSCAL 0x258 while every batch after it uses MSCNT.
    // 0x005fc940.
    void DrawSpritesDmaKicked();
};

/**
 * Allocate and construct a PlayStation 2 particle system.
 *
 * GfxDevice::Init() stores this creator in the system creator hook at `0x0071aef8`.
 *
 * @param name The object name.
 * @return The new system.
 * @ghidraAddress 0x005ffa78
 */
ParticleSys *NewPsParticleSys(const HxStr &name);

} // namespace Rnd
