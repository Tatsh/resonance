#pragma once

#include "math/color.h"
#include "math/vector3.h"

namespace Rnd {

/**
 * One live particle of a Rnd::ParticleSys.
 *
 * The structure is not polymorphic and emits no RTTI descriptor. Its member names come from the
 * labels its own dump writes at `0x00526308`, "\n\tpos:", "\n\tprevPos:", "\n\tvel:", "\n\tcol:",
 * "\n\tcolVel:", "size:", " deathFrame:", and " birthFrame:". The offsets below are the ones that
 * dump reads, so every named member is measured rather than inferred.
 *
 * A particle is 0x80 bytes, which the pool vector stride in Rnd::ParticleSys confirms. Three runs
 * inside it are unrecovered, because the dump reads none of them.
 *
 * Live particles form a singly linked list through mNext rather than occupying the pool vector in
 * order. Rnd::PsParticleSys::DrawSpritesDmaKicked() walks that list, and
 * Rnd::ParticleSys::FreeAllParticles() releases it.
 *
 * The first two quadwords are the pair the vector unit path uploads, which is why the colour
 * precedes the position. The upload writes half the size into the fourth word of mPos first,
 * because a GS sprite takes a centre and a half extent rather than two corners.
 */
struct Particle {
    /** Colour, as the dump label "\n\tcol:" titles it. +0x00 */
    Color mCol;
    /** Rate of colour change per frame. +0x10 */
    Color mColVel;
    /**
     * Position. Its fourth word is overwritten with half of mSize by the vector unit upload.
     *
     * +0x20
     */
    Vector3 mPos;
    /** Position one frame earlier, which the line mode draws from. +0x30 */
    Vector3 mPrevPos;
    /** Velocity per frame. +0x40 */
    Vector3 mVel;
    unsigned char mUnknown50[0x18]; // +0x50 Unrecovered. The particle dump reads none of it.
    /** Extent of the drawn primitive. +0x68 */
    float mSize;
    /** Frame this particle is released on. +0x6c */
    float mDeathFrame;
    /** Frame this particle was allocated on. +0x70 */
    float mBirthFrame;
    int mUnknown74; // +0x74
    /** Next live particle, or null at the end of the list. +0x78 */
    Particle *mNext;
    int mUnknown7c; // +0x7c
};

} // namespace Rnd
