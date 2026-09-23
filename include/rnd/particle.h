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
 * dump reads, so those members are measured rather than inferred. mPrev and the three bubble
 * members are named from their use by Rnd::ParticleSys instead.
 *
 * A particle is 0x80 bytes, which the pool vector stride in Rnd::ParticleSys confirms. One word at
 * the end is unrecovered, because no recovered routine reads it.
 *
 * In line mode Rnd::ParticleSys::UpdateParticles() treats mPrevPos as the first of a run of
 * quadwords and shifts a position history along it, one quadword per unit of line length. A line
 * length above 1 therefore overwrites mVel and the bubble members with older positions.
 *
 * Live particles form a doubly linked list through mNext and mPrev rather than occupying the pool
 * vector in order, and free particles a singly linked list through mNext.
 * Rnd::PsParticleSys::DrawSpritesDmaKicked() walks the live list, and
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
    /**
     * Displacement of the bubble motion, drawn from Rnd::ParticleSys's bubble size range. Inferred
     * from use, because the particle dump does not read it. +0x50
     */
    Vector3 mBubbleSize;
    /** Angular rate of the bubble motion, drawn from the bubble period range. Inferred. +0x60 */
    float mBubbleFrequency;
    /** Phase of the bubble motion. Inferred. +0x64 */
    float mBubblePhase;
    /** Extent of the drawn primitive. +0x68 */
    float mSize;
    /** Frame this particle is released on. +0x6c */
    float mDeathFrame;
    /** Frame this particle was allocated on. +0x70 */
    float mBirthFrame;
    /**
     * Previous live particle, the particle itself at the head of the live list, or null while the
     * particle is free. The particle dump does not read it. The pool routines of Rnd::ParticleSys
     * establish it. +0x74
     */
    Particle *mPrev;
    /** Next live particle, or null at the end of the list. +0x78 */
    Particle *mNext;
    int mUnknown7c; // +0x7c
};

} // namespace Rnd
