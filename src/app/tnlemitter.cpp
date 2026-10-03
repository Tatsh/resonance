#include "app/tnlemitter.h"

#include "math/transformops.h"
#include "rnd/particlesys.h"

// NTSC-U/C: 0x00455d08, PAL: 0x00493238
TnlEmitter::~TnlEmitter() {
    if (mParticleSys != nullptr) {
        mParticleSys->mEmitRateLow = mEmitRateLow;
        mParticleSys->mEmitRateHigh = mEmitRateHigh;
        mParticleSys->SetForce(mForce);
    }
}

// NTSC-U/C: 0x00455d58, PAL: 0x00493288
void TnlEmitter::Attach(Rnd::ParticleSys *pSys) {
    mParticleSys = pSys;
    mEmitRateLow = pSys->mEmitRateLow;
    mEmitRateHigh = pSys->mEmitRateHigh;
    mForce = pSys->GetForce();
    pSys->SetShowing(1);
    Stop();
    pSys->FreeAllParticles();
}

// NTSC-U/C: 0x00455e30, PAL: 0x00493360
void TnlEmitter::RotateForce(const float *pMat3Rows) {
    if (mParticleSys != nullptr) {
        Vector3 force;
        force.w = 1.0f;
        TransformVec3ByMat3VU0(&mForce.x, pMat3Rows, &force.x);
        mParticleSys->SetForce(force);
    }
}

// NTSC-U/C: 0x00455e88, PAL: 0x004933b8
void TnlEmitter::Restart() {
    if (mParticleSys != nullptr) {
        mParticleSys->FreeAllParticles();
        mParticleSys->mEmitRateHigh = mEmitRateHigh;
        mParticleSys->mEmitRateLow = mEmitRateLow;
    }
}

// NTSC-U/C: 0x00455ed0, PAL: 0x00493400
void TnlEmitter::Stop() {
    if (mParticleSys != nullptr) {
        mParticleSys->mEmitRateHigh = 0.0f;
        mParticleSys->mEmitRateLow = 0.0f;
    }
}
