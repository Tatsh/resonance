#include "app/playsound.h"

#include "app/application.h"
#include "os/hxstr.h"
#include "synth/ps2hardsynth.h"

namespace {

// The sound numbers of the powerups that have one.
constexpr int kSoundAutocatcher = 0x3c;
constexpr int kSoundCrippler = 0x3d;
constexpr int kSoundFreestyler = 0x3e;
constexpr int kSoundNeutralizer = 0x3f;
constexpr int kSoundBumper = 0x40;
constexpr int kSoundMultiplier = 0x41;

constexpr int kPowerupSoundUnknown = -1;
constexpr int kPowerupSoundVelocity = 127;

// The values LookupSound() starts its outputs at, and the velocity of the two music loops.
constexpr int kNoNote = -1;
constexpr int kDefaultVelocity = 127;
constexpr int kMusicVelocity = 75;

// The first note StopSoundByName() skips.
constexpr int kSkippedFirstNote = 1;

// A note-off on the last MIDI channel, and the release velocity StopSoundByName() sends.
constexpr unsigned char kNoteOffLastChannel = 0x8f;
constexpr unsigned char kReleaseVelocity = 0;

// The note LookupSound() reports for each registered name.
constexpr int kNoteErase = 23;
constexpr int kNoteEraseSection = 22;
constexpr int kNoteMissPlayer1 = 24;
constexpr int kNoteMissPlayer2 = 25;
constexpr int kNoteMissPlayer3 = 26;
constexpr int kNoteMissPlayer4 = 27;
constexpr int kNoteCaughtPowerup = 31;
constexpr int kNoteLose = 34;
constexpr int kNoteWin = 35;
constexpr int kNoteInactive = 36;
constexpr int kNoteDeployAutocatcher = 72;
constexpr int kNoteDeployCrippler = 73;
constexpr int kNoteDeployFreestyler = 74;
constexpr int kNoteDeployNeutralizer = 75;
constexpr int kNoteDeployBumper = 76;
constexpr int kNoteDeployMultiplier = 77;
constexpr int kNoteCripplerHit = 78;
constexpr int kNoteMultiIngameAction = 84;
constexpr int kNoteMultiIngameNavigation = 85;
constexpr int kNoteMusic1 = 24;
constexpr int kNoteMusic1Second = 25;
constexpr int kNoteMusic2 = 26;
constexpr int kNoteMusic2Second = 27;
constexpr int kNoteHigh = 36;
constexpr int kNoteSlide = 37;
constexpr int kNoteSlideSecond = 38;
constexpr int kNoteCycleLeft = 39;
constexpr int kNoteCycleRight = 40;
constexpr int kNoteKey1 = 41;
constexpr int kNoteKey2 = 42;
constexpr int kNoteLeave = 43;
constexpr int kNoteRandom = 44;
constexpr int kNoteSelectPlayerCount = 49;
constexpr int kNoteSelectArena = 50;
constexpr int kNoteSelectFreq = 51;
constexpr int kNoteSelectLevel = 52;
constexpr int kNoteSelectMode = 53;
constexpr int kNoteSelectRemix = 54;
constexpr int kNoteSelectSkill = 55;
constexpr int kNoteSoloMulti = 56;
constexpr int kNoteFrequency = 57;
constexpr int kNoteFreqMakerToggle = 60;
constexpr int kNoteFreqMakerPartMove = 61;
constexpr int kNoteFreqMakerPartSelect = 62;
constexpr int kNoteFreqMakerFlipBubble = 63;
constexpr int kNoteFreqMakerColorMove = 64;
constexpr int kNoteFreqMakerDelete = 45;
constexpr int kNoteError = 45;

} // namespace

// 0x0012e570
void LookupSound(const HxStr &name, int *pNote, int *pNote2, int *pVelocity, int *pAutoStop) {
    *pNote = kNoNote;
    *pNote2 = kNoNote;
    *pVelocity = kDefaultVelocity;
    *pAutoStop = 0;
    if (name == "SND_ERASE") {
        *pNote = kNoteErase;
    } else if (name == "SND_ERASE_SECTION") {
        *pNote = kNoteEraseSection;
        *pAutoStop = 1;
    } else if (name == "SND_MISS_PLAYER1") {
        *pNote = kNoteMissPlayer1;
    } else if (name == "SND_MISS_PLAYER2") {
        *pNote = kNoteMissPlayer2;
    } else if (name == "SND_MISS_PLAYER3") {
        *pNote = kNoteMissPlayer3;
    } else if (name == "SND_MISS_PLAYER4") {
        *pNote = kNoteMissPlayer4;
    } else if (name == "SND_CAUGHT_POWERUP") {
        *pNote = kNoteCaughtPowerup;
        *pAutoStop = 1;
    } else if (name == "SND_LOSE") {
        *pNote = kNoteLose;
    } else if (name == "SND_WIN") {
        *pNote = kNoteWin;
        *pAutoStop = 1;
    } else if (name == "SND_INACTIVE") {
        *pNote = kNoteInactive;
    } else if (name == "SND_DEPLOY_AUTOCATCHER") {
        *pNote = kNoteDeployAutocatcher;
        *pAutoStop = 1;
    } else if (name == "SND_DEPLOY_CRIPPLER") {
        *pNote = kNoteDeployCrippler;
        *pAutoStop = 1;
    } else if (name == "SND_DEPLOY_FREESTYLER") {
        *pNote = kNoteDeployFreestyler;
        *pAutoStop = 1;
    } else if (name == "SND_DEPLOY_NEUTRALIZER") {
        *pNote = kNoteDeployNeutralizer;
        *pAutoStop = 1;
    } else if (name == "SND_DEPLOY_BUMPER") {
        *pNote = kNoteDeployBumper;
        *pAutoStop = 1;
    } else if (name == "SND_DEPLOY_MULTIPLIER") {
        *pNote = kNoteDeployMultiplier;
        *pAutoStop = 1;
    } else if (name == "SND_CRIPPLER_HIT") {
        *pNote = kNoteCripplerHit;
        *pAutoStop = 1;
    } else if (name == "SND_MET_MULTI_INGAME_ACTION") {
        *pNote = kNoteMultiIngameAction;
    } else if (name == "SND_MET_MULTI_INGAME_NAVIGATION") {
        *pNote = kNoteMultiIngameNavigation;
    } else if (name == "SND_MET_MUSIC1") {
        *pNote = kNoteMusic1;
        *pNote2 = kNoteMusic1Second;
        *pVelocity = kMusicVelocity;
    } else if (name == "SND_MET_MUSIC2") {
        *pNote = kNoteMusic2;
        *pNote2 = kNoteMusic2Second;
        *pVelocity = kMusicVelocity;
    } else if (name == "SND_MET_HIGH") {
        *pNote = kNoteHigh;
    } else if (name == "SND_MET_SLIDE") {
        *pNote = kNoteSlide;
        *pNote2 = kNoteSlideSecond;
    } else if (name == "SND_MET_CYCLE_L") {
        *pNote = kNoteCycleLeft;
    } else if (name == "SND_MET_CYCLE_R") {
        *pNote = kNoteCycleRight;
    } else if (name == "SND_MET_KEY1") {
        *pNote = kNoteKey1;
    } else if (name == "SND_MET_KEY2") {
        *pNote = kNoteKey2;
    } else if (name == "SND_MET_LEAVE") {
        *pNote = kNoteLeave;
    } else if (name == "SND_MET_RANDOM") {
        *pNote = kNoteRandom;
    } else if (name == "SND_MET_SELECTNUMPLAYER") {
        *pNote = kNoteSelectPlayerCount;
    } else if (name == "SND_MET_SELECTARENA") {
        *pNote = kNoteSelectArena;
    } else if (name == "SND_MET_SELECTFREQ") {
        *pNote = kNoteSelectFreq;
    } else if (name == "SND_MET_SELECTLEVEL") {
        *pNote = kNoteSelectLevel;
    } else if (name == "SND_MET_SELECTMODE") {
        *pNote = kNoteSelectMode;
    } else if (name == "SND_MET_SELECTREMIX") {
        *pNote = kNoteSelectRemix;
    } else if (name == "SND_MET_SELECTSKILL") {
        *pNote = kNoteSelectSkill;
    } else if (name == "SND_MET_SOLOMULTI") {
        *pNote = kNoteSoloMulti;
    } else if (name == "SND_MET_FREQUENCY") {
        *pNote = kNoteFrequency;
    } else if (name == "SND_MET_FM_TOGGLE") {
        *pNote = kNoteFreqMakerToggle;
    } else if (name == "SND_MET_FM_PART_MOVE") {
        *pNote = kNoteFreqMakerPartMove;
    } else if (name == "SND_MET_FM_PART_SELECT") {
        *pNote = kNoteFreqMakerPartSelect;
    } else if (name == "SND_MET_FM_FLIP_BUBBLE") {
        *pNote = kNoteFreqMakerFlipBubble;
    } else if (name == "SND_MET_FM_COLOR_MOVE") {
        *pNote = kNoteFreqMakerColorMove;
    } else if (name == "SND_MET_FM_DELETE") {
        *pNote = kNoteFreqMakerDelete;
    } else if (name == "SND_MET_ERROR") {
        *pNote = kNoteError;
    }
}

// 0x0012eba0
void StopSoundByName(const char *pszName) {
    const HxStr name(pszName);
    int nNote = kNoNote;
    int nNote2 = kNoNote;
    int nVelocity = kDefaultVelocity;
    int bAutoStop = 0;
    LookupSound(name, &nNote, &nNote2, &nVelocity, &bAutoStop);
    Ps2HardSynth *pSynth = Application::shared()->GetSynth();
    // Yes, the binary tests the first note against 1 rather than -1.
    if (nNote != kSkippedFirstNote) {
        pSynth->SendMidi(kNoteOffLastChannel, nNote, kReleaseVelocity);
    }
    if (nNote2 != kNoNote) {
        pSynth->SendMidi(kNoteOffLastChannel, nNote2, kReleaseVelocity);
    }
}

// 0x0012f520
void PlayPowerupSound(HudItemKind kind) {
    int nSound;
    switch (kind) {
    case kHudItemNeutralizer:
        nSound = kSoundNeutralizer;
        break;
    case kHudItemCrippler:
        nSound = kSoundCrippler;
        break;
    case kHudItemFreestyler:
        nSound = kSoundFreestyler;
        break;
    case kHudItemAutocatcher:
        nSound = kSoundAutocatcher;
        break;
    case kHudItemBumper:
        nSound = kSoundBumper;
        break;
    case kHudItemMultiplier:
        nSound = kSoundMultiplier;
        break;
    default:
        return;
    }
    PlaySynthSound(nSound, kPowerupSoundUnknown, kPowerupSoundVelocity, 0);
}
