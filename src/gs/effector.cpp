#include "gs/effector.h"

#include <vector>

#include "app/application.h"
#include "gs/ghostnoteseffector.h"
#include "gs/midionoffeffector.h"
#include "gs/stuttereffector.h"
#include "gs/volumeeffector.h"
#include "gs/waheffector.h"
#include "msg/message.h"
#include "os/log.h"
#include "script/configquery.h"

namespace {

// The configuration codes the factory reads the tuning values from.
constexpr int kVolumeConfigCode = 0x392;
constexpr int kWahDepthConfigCode = 0x390;
constexpr int kWahPeriodConfigCode = 0x38f;
constexpr int kStutterConfigCode = 0x391;

constexpr unsigned int kStutterParameterCount = 2;

// The controllers the three MidiOnOffEffector types switch.
constexpr unsigned char kFirstSwitchController = 0x52;
constexpr unsigned char kSecondSwitchController = 0x53;
constexpr unsigned char kThirdSwitchController = 0x51;

} // namespace

// 0x001a0df0
Effector *Effector::CreateForType(int nType, unsigned char nChannel, int nTrack) {
    Effector *pEffector = nullptr;
    Sch::TickClock *pClock = Application::shared()->GetSongClock();

    switch (nType) {
    case kEffectorTypeVolume:
        pEffector = new VolumeEffector(nChannel, QueryConfigValue(kVolumeConfigCode));
        break;
    case kEffectorTypeWah: {
        const int nDepth = QueryConfigValue(kWahDepthConfigCode, nTrack);
        const int nPeriod = QueryConfigValue(kWahPeriodConfigCode, nTrack);
        pEffector = new WahEffector(pClock, nChannel, nDepth, nPeriod);
        break;
    }
    case kEffectorTypeStutter: {
        std::vector<int> parameters;
        QueryConfigVector(&parameters, kStutterConfigCode);
        if (parameters.size() != kStutterParameterCount) {
            Fatal("Need 2 stutter parameters");
        }
        pEffector = new StutterEffector(pClock, nChannel, parameters[0], parameters[1]);
        break;
    }
    case kEffectorTypeMidiOnOffFirst:
        pEffector = new MidiOnOffEffector(nChannel, nType, kFirstSwitchController);
        break;
    case kEffectorTypeMidiOnOffSecond:
        pEffector = new MidiOnOffEffector(nChannel, nType, kSecondSwitchController);
        break;
    case kEffectorTypeMidiOnOffThird:
        pEffector = new MidiOnOffEffector(nChannel, nType, kThirdSwitchController);
        break;
    case kEffectorTypeGhostNotes:
        pEffector = new GhostNotesEffector;
        break;
    default:
        break;
    }

    // Yes, the binary discards the result, and a type outside the table dereferences null here.
    pEffector->Type();
    return pEffector;
}

// 0x001a18c8
Effector::Effector() {
}

// 0x001a24a8
Effector::~Effector() {
}

// 0x001a2558
void Effector::Enable([[maybe_unused]] int bEnabled) {
}
