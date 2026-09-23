#include "met/metnullrenderer.h"

#include "app/application.h"
#include "app/mainloop.h"
#include "app/playsound.h"
#include "app/renderer.h"
#include "game/gamemanagerimpl.h"
#include "game/globalsettings.h"
#include "game/grooveworld.h"
#include "met/metfreqmakerassetmanager.h"
#include "met/metpersonadata.h"
#include "met/metsonglists.h"
#include "msg/begingamelocalmsg.h"
#include "msg/metcontrollerreading.h"
#include "msg/rawcontrollermsg.h"
#include "msg/unpausegamesystemmsg.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "os/log.h"
#include "os/r250.h"
#include "script/cxx/int.h"
#include "script/cxx/seqbase.h"
#include "script/cxx/string.h"
#include "script/scripteval.h"

namespace {

constexpr int kRandomSeed = 123439;

// The tag of a joystick reading, `joy `.
constexpr int kReadingTagJoystick = 0x6a6f7920;

// The X button, from the warning text. Button 4's identity is not recovered.
constexpr int kButtonX = 3;
constexpr int kButtonUnload = 4;

// The script template that yields the ruleset sequence.
constexpr int kRulesetTemplate = 204;

enum RulesetField {
    kRulesetFieldLevel = 0,
    kRulesetFieldMode = 1,
    kRulesetFieldDifficulty = 2,
    kRulesetFieldPlayerCount = 3,
    kRulesetFieldArena = 4,
};

constexpr int kSinglePlayer = 1;

static const char *const kXButtonWarning = "X button";
static const char *const kSlideSound = "SND_MET_SLIDE";
static const char *const kJamRuleset = "jam";
static const char *const kGameRuleset = "game";
static const char *const kBadRulesetFormat = "Cannot parse Ruleset: %s";
static const char *const kPersonaNameFormat = "freq player%d";

} // namespace

// 0x0030e2c8
MetNullRenderer::MetNullRenderer() : mUnknown80(0) {
    SeedR250(kRandomSeed);
    MetFreqMakerAssetManager::Create();
    MetFreqMakerAssetManager::Instance()->StartAssetLoad();
    GlobalSettings::Create();
    Application::shared()->GetGameManager()->SetDrawEnabled(1);
    RebuildStageLists();
}

// 0x0030e400
MetNullRenderer::~MetNullRenderer() {
    MetFreqMakerAssetManager::Destroy();
}

// 0x0030e4c0
void MetNullRenderer::OnRawController(RawControllerMsg *pMsg) {
    const MetControllerReading &reading = pMsg->mReading;
    if (reading.mTag != kReadingTagJoystick || !(reading.mValue > 0.0f)) {
        return;
    }

    switch (reading.mButton) {
    case kButtonX: {
        Warn(kXButtonWarning);
        GrooveWorld *pWorld = Application::shared()->GetGameManager()->GetWorld();
        if (pWorld != nullptr) {
            UnpauseGameSystemMsg msg;
            Application::shared()->GetGameManager()->QueueMessage(&msg);
            pWorld->PostExitMode2();
            break;
        }

        PlaySoundByName(kSlideSound);
        Py::Sequence ruleset(EvalScriptTemplate(kRulesetTemplate));
        mUnknown48.mLevelName = Py::String(ruleset[kRulesetFieldLevel]);

        mUnknown48.mUnknown1c = ParseRuleset(Py::String(ruleset[kRulesetFieldMode]));
        mUnknown48.mDifficulty = Py::Int(ruleset[kRulesetFieldDifficulty]);
        mUnknown48.mUnknown28 = 0;
        mUnknown48.mArenaName = Py::String(ruleset[kRulesetFieldArena]);
        mUnknown84 = Py::Int(ruleset[kRulesetFieldPlayerCount]);

        Renderer::LoadCommon();
        Renderer::LoadLevel(mUnknown48);
        mUnknown80 = 1;
        break;
    }
    case kButtonUnload:
        Renderer::UnloadLevel();
        Renderer::UnloadCommon();
        break;
    }
}

// 0x0030f320
void MetNullRenderer::OnUnknownSlot8() {
    float flCommonProgress;
    float flLevelProgress;
    const int nCommonDone = Renderer::PollCommon(&flCommonProgress);
    const int nLevelDone = Renderer::PollLevel(&flLevelProgress);
    if (mUnknown80 != 0 && nCommonDone != 0 && nLevelDone != 0) {
        Application::shared()->GetGameManager()->ClearPersonas();
        for (int nPlayer = 0; nPlayer < mUnknown84; ++nPlayer) {
            MetPersonaData persona;
            persona.mUnknown140.mUnknown00 = HxStr(FormatString(kPersonaNameFormat, nPlayer));
            Application::shared()->GetGameManager()->AddPersona(persona);
        }
        Application::shared()->GetGameManager()->SetGameMode(
            mUnknown84 == kSinglePlayer ? kGameModeSolo : kGameModeLocal);
        Application::shared()->GetGameManager()->SetParams(mUnknown48);
        BeginGameLocalMsg msg;
        Application::shared()->GetGameManager()->QueueMessage(&msg);
        mUnknown80 = 0;
    }

    if (Application::shared()->GetGameManager()->GetWorld() == nullptr) {
        MainLoop::PumpTimers();
    }
}

// 0x00311a20
void MetNullRenderer::OnUnknownSlot7() {
}

// 0x00311f80
int MetNullRenderer::ParseRuleset(const HxStr &ruleset) {
    if (ruleset == kJamRuleset) {
        return kPlayModeJam;
    }
    if (ruleset == kGameRuleset) {
        return kPlayModeGame;
    }
    Fatal(kBadRulesetFormat, ruleset.mStr != nullptr ? ruleset.mStr : g_szEmptyString);
    return kPlayModeNone; // Yes, the binary returns after Fatal().
}

// 0x00311ff0
void MetNullRenderer::HandleMessage(Message *pMsg) {
    if (pMsg->Type() == g_nRawControllerMsgType) {
        OnRawController(static_cast<RawControllerMsg *>(pMsg));
    }
}
