#pragma once

#include "math/vector3.h"

namespace Rnd {
class ParticleSys;
} // namespace Rnd

/**
 * Handle on a loaded particle system that can silence it and put back its authored settings.
 *
 * The class is not polymorphic and emits no RTTI, and no allocation tag or file path identifies
 * it. The name is inferred from what it records, the emission rate range and the force of one
 * Rnd::ParticleSys. TnlArms, TnlMultFX, and TnlFireFX embed it by value, 0x20 bytes each.
 *
 * Attach() records the authored rates and force and silences the system. Restart() gives the rates
 * back and Stop() zeroes them again. Destruction writes the recorded rates and force back.
 */
class TnlEmitter {
public:
    /**
     * Start with no system.
     *
     * The body has no out-of-line copy. Every owner inlines it.
     */
    TnlEmitter() : mParticleSys(nullptr) {
        mForce.w = 1.0f;
    }

    /**
     * Write the recorded rates and force back to the system.
     *
     * @ghidraAddress NTSC-U/C: 0x00455d08
     * @ghidraAddress PAL: 0x00493238
     */
    ~TnlEmitter();

    /**
     * Record the rates and force of pSys, show it, silence it, and release its live particles.
     *
     * pSys must not be null.
     *
     * @param pSys The particle system.
     * @ghidraAddress NTSC-U/C: 0x00455d58
     * @ghidraAddress PAL: 0x00493288
     */
    void Attach(Rnd::ParticleSys *pSys);

    /**
     * Give the system the recorded force turned by a rotation.
     *
     * @param pMat3Rows The rotation, three rows of four floats.
     * @ghidraAddress NTSC-U/C: 0x00455e30
     * @ghidraAddress PAL: 0x00493360
     */
    void RotateForce(const float *pMat3Rows);

    /**
     * Release the live particles and give back the recorded rates.
     *
     * @ghidraAddress NTSC-U/C: 0x00455e88
     * @ghidraAddress PAL: 0x004933b8
     */
    void Restart();

    /**
     * Zero both emission rates.
     *
     * @ghidraAddress NTSC-U/C: 0x00455ed0
     * @ghidraAddress PAL: 0x00493400
     */
    void Stop();

    /**
     * Report the system.
     *
     * The owners read the field directly, and no out-of-line copy exists.
     *
     * @return The system, or null before Attach().
     */
    Rnd::ParticleSys *GetParticleSys() const {
        return mParticleSys;
    }

private:
    Rnd::ParticleSys *mParticleSys;
    float mEmitRateLow;
    float mEmitRateHigh;
    Vector3 mForce;
};
