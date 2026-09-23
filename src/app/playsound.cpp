#include "app/playsound.h"

#include <algorithm>

#include "app/application.h"
#include "app/globals.h"
#include "app/ticktask.h"
#include "mid/mbt.h"
#include "os/hxstr.h"
#include "sch/tickclock.h"
#include "synth/ps2hardsynth.h"

namespace {

// The sound numbers of the powerups that have one.
constexpr int kSoundAutocatcher = 0x3c;
constexpr int kSoundCrippler = 0x3d;
constexpr int kSoundFreestyler = 0x3e;
constexpr int kSoundNeutralizer = 0x3f;
constexpr int kSoundBumper = 0x40;
constexpr int kSoundMultiplier = 0x41;

constexpr int kPowerupSoundVelocity = 127;

// The note PlayActivateSound() plays.
constexpr int kNoteActivate = 100;

// A note-on on the last MIDI channel, and the ticks PlaySynthSound() lets an auto-stop note sound.
constexpr unsigned char kNoteOnLastChannel = 0x9f;
constexpr int kAutoStopTicks = 480;

// The note destroyer runs every 120 ticks and can queue as many notes as MIDI has.
constexpr int kDestroyerPeriodTicks = 120;
constexpr int kDestroyerCapacity = 128;
// The task runs at once rather than at the next multiple of its period.
constexpr int kDestroyerUnaligned = 0;
// The task asks to be run again after every pass.
constexpr int kRunAgain = 1;

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

/**
 * Task that releases the auto-stop notes PlaySynthSound() starts.
 *
 * `NoteDestroyer` in the anonymous namespace of `AppPlaySoundPS2.cpp`, as its RTTI name records,
 * with TickTask as its one base. The vtable is at `0x007d1b18`, and the object is 0x42c bytes.
 * CreateNoteDestroyer()
 * expands the constructor, whose uncalled out-of-line copy is at `0x0012f1e0`.
 */
class NoteDestroyer : public TickTask {
public:
    /**
     * @param pGlobals The globals whose song clock and synthesiser the task uses.
     */
    explicit NoteDestroyer(Globals *pGlobals)
        : TickTask(
              pGlobals->GetSongClock(), Mid::MBT(kDestroyerPeriodTicks).mTick, kDestroyerUnaligned),
          mGlobals(pGlobals), mSynth(pGlobals->GetSynth()), mCount(0) {
        for (int nIndex = 0; nIndex < kDestroyerCapacity; ++nIndex) {
            mEntries[nIndex].mTick = kMBTInfinity;
        }
    }

    /** @ghidraAddress 0x0012f2b8 */
    virtual ~NoteDestroyer() {
    }

    /**
     * Send a note-off for every queued note whose release tick has arrived, and drop it.
     *
     * A released entry is replaced by the last one, which is then tested in its place.
     *
     * @param nTick The song position.
     * @return Always 1, to run again.
     * @ghidraAddress 0x0012f308
     */
    virtual int Tick(int nTick) {
        for (int nIndex = 0; nIndex < mCount;) {
            if (!(nTick < mEntries[nIndex].mTick)) {
                mSynth->SendMidi(kNoteOffLastChannel, mEntries[nIndex].mNote, kReleaseVelocity);
                --mCount;
                mEntries[nIndex] = mEntries[mCount];
            } else {
                ++nIndex;
            }
        }
        return kRunAgain;
    }

    /**
     * Queue a note for release. The capacity is not checked.
     *
     * @param nNote The note.
     * @param nTick The song position to release it at.
     */
    void Add(int nNote, int nTick) {
        mEntries[mCount].mNote = nNote;
        mEntries[mCount].mTick = nTick;
        ++mCount;
    }

private:
    // One queued release.
    struct Entry {
        int mNote;
        int mTick;
    };

    Globals *mGlobals;                  // +0x20
    Ps2HardSynth *mSynth;               // +0x24
    Entry mEntries[kDestroyerCapacity]; // +0x28
    int mCount;                         // +0x428
};

// The note destroyer, from CreateNoteDestroyer() to DestroyNoteDestroyer().
// 0x0066f538
NoteDestroyer *g_pNoteDestroyer;

} // namespace

// 0x0012e460
void CreateNoteDestroyer() {
    g_pNoteDestroyer = new NoteDestroyer(Application::shared());
}

// 0x0012ea50
void PlaySynthSound(int nNote, int nNote2, int nVelocity, int bAutoStop) {
    Ps2HardSynth *pSynth = Application::shared()->GetSynth();
    // Yes, the binary tests the first note against 1 rather than -1.
    if (nNote != kSkippedFirstNote) {
        pSynth->SendMidi(kNoteOnLastChannel, nNote, nVelocity);
        if (bAutoStop != 0) {
            NoteDestroyer *pDestroyer = g_pNoteDestroyer;
            const int nNow = Application::shared()->GetSongClock()->SongTick();
            const Mid::MBT release(std::min(
                std::max(nNow + Mid::MBT(kAutoStopTicks).mTick, kMBTMinimum), kMBTMaximum));
            pDestroyer->Add(nNote, release.mTick);
        }
    }
    if (nNote2 != kNoNote) {
        pSynth->SendMidi(kNoteOnLastChannel, nNote2, nVelocity);
    }
}

// 0x0012f3d8
void StartNoteDestroyer() {
    g_pNoteDestroyer->Start(kMBTInfinity);
}

// 0x0012f400
void StopNoteDestroyer() {
    g_pNoteDestroyer->Stop();
}

// 0x0012f428
void DestroyNoteDestroyer() {
    delete g_pNoteDestroyer;
    g_pNoteDestroyer = nullptr;
}

// 0x0012f470
void PlaySoundByName(const char *pszName) {
    const HxStr name(pszName);
    int nNote = kNoNote;
    int nNote2 = kNoNote;
    int nVelocity = kDefaultVelocity;
    int bAutoStop = 0;
    LookupSound(name, &nNote, &nNote2, &nVelocity, &bAutoStop);
    PlaySynthSound(nNote, nNote2, nVelocity, bAutoStop);
}

// 0x0012f598
void PlayActivateSound() {
    PlaySynthSound(kNoteActivate, kNoNote, kDefaultVelocity, 0);
}

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
    PlaySynthSound(nSound, kNoNote, kPowerupSoundVelocity, 0);
}
