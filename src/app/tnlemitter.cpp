#include "app/tnlemitter.h"

#include "math/transformops.h"
#include "rnd/particlesys.h"

TnlEmitter::~TnlEmitter() {
    if (mParticleSys != nullptr) {
        mParticleSys->mEmitRateLow = mEmitRateLow;
        mParticleSys->mEmitRateHigh = mEmitRateHigh;
        mParticleSys->SetForce(mForce);
    }
}

void TnlEmitter::Attach(Rnd::ParticleSys *pSys) {
    mParticleSys = pSys;
    mEmitRateLow = pSys->mEmitRateLow;
    mEmitRateHigh = pSys->mEmitRateHigh;
    mForce = pSys->GetForce();
    pSys->SetShowing(1);
    Stop();
    pSys->FreeAllParticles();
}

void TnlEmitter::RotateForce(const float *pMat3Rows) {
    if (mParticleSys != nullptr) {
        Vector3 force;
        force.w = 1.0f;
        TransformVec3ByMat3VU0(&mForce.x, pMat3Rows, &force.x);
        mParticleSys->SetForce(force);
    }
}

void TnlEmitter::Restart() {
    if (mParticleSys != nullptr) {
        mParticleSys->FreeAllParticles();
        mParticleSys->mEmitRateHigh = mEmitRateHigh;
        mParticleSys->mEmitRateLow = mEmitRateLow;
    }
}

void TnlEmitter::Stop() {
    if (mParticleSys != nullptr) {
        mParticleSys->mEmitRateHigh = 0.0f;
        mParticleSys->mEmitRateLow = 0.0f;
    }
}
