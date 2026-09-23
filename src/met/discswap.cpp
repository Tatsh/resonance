#include "met/discswap.h"

#include <libcdvd.h>

#include "os/arkfile.h"
#include "os/async.h"
#include "os/hostmode.h"
#include "script/configquery.h"
#include "script/scripthost.h"

namespace {

// Polls of an unready drive before the insert sequence classifies the disc regardless.
constexpr int kPollLimit = 1800;

// Configuration value 0x514, the one GetAlbumJukeboxValue() also reads.
constexpr int kConfigCode = 1300;

// The script template MountDisc() runs after the archives are mounted again.
constexpr int kMountedTemplate = 205;

// sceCdTrayReq() reports success with 1.
constexpr int kTrayRequestDone = 1;

} // namespace

// 0x0016a048
void DiscSwap::Reset() {
    polls_ = 0;
    pollLimit_ = kPollLimit;
    discOnly_ = GetHostMode() == kHostModeCdOnly;
    configValue_ = QueryConfigValue(kConfigCode);
    noDisc_ = 0;
}

// 0x0016a098
int DiscSwap::ReleaseDisc() {
    int nPending;
    int nCompleted;
    int nFreeJobs;
    CountAsyncQueues(&nPending, &nCompleted, &nFreeJobs);
    if (nPending > 0) {
        AsyncPumpCompletedRequests();
        return 0;
    }
    return CloseArk() != 0;
}

// 0x0016a168
void DiscSwap::BeginEject() {
    phase_ = kPhaseStart;
}

// 0x00169e50
int DiscSwap::PollEject() {
    if (discOnly_ == 0) {
        phase_ = kPhaseOpened;
        return kStepTrayOpen;
    }

    if (phase_ == kPhaseStart) {
        if (sceCdGetDiskType() == SCECdNODISC || sceCdStop() != 0) {
            phase_ = kPhaseStopping;
            return kStepEjecting;
        }
        phase_ = kPhaseStart;
        return kStepEjectFailed;
    }

    if (phase_ == kPhaseStopping) {
        if (sceCdSync(SCECdNonblock) == 0) {
            phase_ = kPhaseOpenTray;
        }
        return kStepEjecting;
    }

    if (phase_ == kPhaseOpenTray) {
        // Yes, the binary leaves the phase at kPhaseOpenTray once the tray opens.
        if (sceCdTrayReq(SCECdTrayOpen, nullptr) != 0) {
            return kStepTrayOpen;
        }
        phase_ = kPhaseStart;
        return kStepEjectFailed;
    }

    return kStepEjecting;
}

// 0x0016a170
void DiscSwap::BeginInsert() {
    phase_ = kPhaseStart;
}

// 0x00169f08
int DiscSwap::PollInsert() {
    if (discOnly_ == 0) {
        phase_ = kPhaseClassify;
        return kStepDiscReady;
    }

    if (phase_ == kPhaseStart) {
        if (sceCdSync(SCECdNonblock) == 0) {
            phase_ = kPhaseCloseTray;
        }
        return kStepClosing;
    }

    if (phase_ == kPhaseCloseTray) {
        if (sceCdTrayReq(SCECdTrayClose, nullptr) != kTrayRequestDone) {
            return kStepCloseFailed;
        }
        polls_ = 0;
        phase_ = kPhaseSpinUp;
        return kStepClosing;
    }

    if (phase_ == kPhaseSpinUp) {
        if (pollLimit_ < polls_) {
            phase_ = kPhaseClassify;
        }
        if (sceCdDiskReady(SCECdNonblock) == SCECdComplete) {
            phase_ = kPhaseClassify;
            return kStepClosing;
        }
        ++polls_;
    }

    if (phase_ != kPhaseClassify) {
        return kStepClosing;
    }
    return ClassifyDisc();
}

// 0x0016a0e8
int DiscSwap::CheckDisc() {
    // Yes, the binary reports kStepTrayOpen when the game is not reading from the disc alone.
    if (discOnly_ == 0 || sceCdDiskReady(SCECdNonblock) != SCECdComplete) {
        return kStepTrayOpen;
    }
    return ClassifyDisc();
}

// 0x0016a178
int DiscSwap::MountDisc() {
    if (discOnly_ != 0 && sceCdDiskReady(SCECdNonblock) != SCECdComplete) {
        return kStepNotReady;
    }
    if (InitArk() == 0) {
        return kStepBadDisc;
    }
    CallScriptTemplate(kMountedTemplate);
    return kStepMounted;
}

inline int DiscSwap::ClassifyDisc() {
    const int nType = sceCdGetDiskType();
    if (nType == SCECdNODISC) {
        noDisc_ = 1;
        return kStepNoDisc;
    }
    if (nType == SCECdPS2DVD || nType == SCECdPS2CD || nType == SCECdPS2CDDA) {
        return kStepDiscReady;
    }
    return kStepBadDisc;
}
