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

// NTSC-U/C: 0x0030e2c8, PAL: 0x00333f20
MetNullRenderer::MetNullRenderer() : mLoadPending(0) {
    SeedR250(kRandomSeed);
    MetFreqMakerAssetManager::Create();
    MetFreqMakerAssetManager::Instance()->StartAssetLoad();
    GlobalSettings::Create();
    Application::shared()->GetGameManager()->SetDrawEnabled(1);
    RebuildStageLists();
}

// NTSC-U/C: 0x0030e400, PAL: 0x00334090
MetNullRenderer::~MetNullRenderer() {
    MetFreqMakerAssetManager::Destroy();
}

// NTSC-U/C: 0x0030e4c0, PAL: 0x00334188
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
            pWorld->PostQuit();
            break;
        }

        PlaySoundByName(kSlideSound);
        Py::Sequence ruleset(EvalScriptTemplate(kRulesetTemplate));
        mParams.mLevelName = Py::String(ruleset[kRulesetFieldLevel]);

        mParams.mPlayMode = ParseRuleset(Py::String(ruleset[kRulesetFieldMode]));
        mParams.mDifficulty = Py::Int(ruleset[kRulesetFieldDifficulty]);
        mParams.mNetGame = 0;
        mParams.mArenaName = Py::String(ruleset[kRulesetFieldArena]);
        mPlayerCount = Py::Int(ruleset[kRulesetFieldPlayerCount]);

        Renderer::LoadCommon();
        Renderer::LoadLevel(mParams);
        mLoadPending = 1;
        break;
    }
    case kButtonUnload:
        Renderer::UnloadLevel();
        Renderer::UnloadCommon();
        break;
    }
}

// NTSC-U/C: 0x0030f320, PAL: 0x00335038
void MetNullRenderer::Draw() {
    float flCommonProgress;
    float flLevelProgress;
    const int nCommonDone = Renderer::PollCommon(&flCommonProgress);
    const int nLevelDone = Renderer::PollLevel(&flLevelProgress);
    if (mLoadPending != 0 && nCommonDone != 0 && nLevelDone != 0) {
        Application::shared()->GetGameManager()->ClearPersonas();
        for (int nPlayer = 0; nPlayer < mPlayerCount; ++nPlayer) {
            MetPersonaData persona;
            persona.mAppearance.mUserName = HxStr(FormatString(kPersonaNameFormat, nPlayer));
            Application::shared()->GetGameManager()->AddPersona(persona);
        }
        Application::shared()->GetGameManager()->SetGameMode(
            mPlayerCount == kSinglePlayer ? kGameModeSolo : kGameModeLocal);
        Application::shared()->GetGameManager()->SetParams(mParams);
        BeginGameLocalMsg msg;
        Application::shared()->GetGameManager()->QueueMessage(&msg);
        mLoadPending = 0;
    }

    if (Application::shared()->GetGameManager()->GetWorld() == nullptr) {
        MainLoop::PumpTimers();
    }
}

// NTSC-U/C: 0x00311a20, PAL: 0x00337780
void MetNullRenderer::Update() {
}

// NTSC-U/C: 0x00311f80, PAL: 0x00337ce0
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

// NTSC-U/C: 0x00311ff0, PAL: 0x00337d50
void MetNullRenderer::HandleMessage(Message *pMsg) {
    if (pMsg->Type() == g_nRawControllerMsgType) {
        OnRawController(static_cast<RawControllerMsg *>(pMsg));
    }
}
