#include "game/forcefeedbackmgr.h"

#include <algorithm>
#include <iostream>
#include <vector>

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/inputpoller.h"
#include "game/player.h"
#include "sch/command.h"
#include "sch/tempomap.h"
#include "sch/tickclock.h"
#include "script/configquery.h"

namespace {

// The bits of ForceFeedbackMgr::mFlags. Any set bit suspends vibration.
constexpr unsigned char kFlagTooManyPlayers = 0x01;
constexpr unsigned char kFlagJukebox = 0x02;
constexpr unsigned char kFlagPlayback = 0x04;
constexpr unsigned char kFlagPaused = 0x08;
constexpr unsigned char kFlagStopped = 0x10;
constexpr unsigned char kFlagDisabled = 0x20;

// SetPlayerCount() suspends vibration above this many players.
constexpr int kMaxVibratingPlayers = 2;

// PlayEffect() ignores a player without a slot.
constexpr int kNoPlayerSlot = -1;

// One bar and one beat at 480 ticks per quarter note.
constexpr int kBarTicks = 1920;
constexpr int kBeatTicks = 480;

// The configuration codes LoadConfig() reads.
constexpr int kMetronomeQuery = 1201;
constexpr int kEffectCount = 5;

// SyncMetronome() rounds the pulse to milliseconds and adds this lead before converting to ticks.
constexpr long long kNanosecondsPerMillisecond = 1000000;
constexpr long long kHalfMillisecondNs = 500000;
constexpr int kMotorLeadNs = 90000000;

// The effect numbers the one-line wrappers pass.
constexpr int kEffectAutocatch = 0;
constexpr int kEffectBump = 1;
constexpr int kEffectCripple = 2;
constexpr int kEffectNeutralized = 3;
constexpr int kEffectUnused = 4;

// Motor states.
constexpr int kMotorOff = 0;
constexpr int kSmallMotorOn = 1;
constexpr int kPowerupRunning = 1;
constexpr int kPowerupDone = 0;

// NTSC-U/C: 0x0067a3f0, PAL: 0x006bb340
// The instance the commands run against, set by the constructor.
ForceFeedbackMgr *g_pForceFeedbackMgr;

// The clamp the inline Sch::Tick arithmetic applies to a computed position.
inline Sch::Tick MakePosition(int nTick) {
    return Sch::Tick(std::min(std::max(nTick, kTickMinimum), kTickMaximum));
}

inline Sch::TickClock *SongClock() {
    return Application::shared()->GetSongClock();
}

/**
 * Scheduler command that runs ForceFeedbackMgr::PulseBeat() on the beat.
 *
 * `Q237_GLOBAL_$N$ForceFeedbackMgr.cppXFKhgb11SteadyFBCmd` in the RTTI, with its vtable at
 * `0x007d9168`. The destructor at `0x00170038` is implicitly declared.
 */
class SteadyFBCmd : public Sch::Command {
public:
    // NTSC-U/C: 0x001700d8, PAL: 0x001729e8
    virtual int CmdID() {
        return sCmdID;
    }

    // NTSC-U/C: 0x001700b0, PAL: 0x001729c0
    virtual void Execute() {
        g_pForceFeedbackMgr->PulseBeat();
    }

    // NTSC-U/C: 0x001700f8, PAL: 0x00172a08
    virtual void Print(std::ostream &stream) {
        stream << "{" << "SteadyFBCmd" << "}";
    }

    // NTSC-U/C: 0x001700e8, PAL: 0x001729f8
    virtual void saveGuts([[maybe_unused]] OBStream &stream) const {
    }

    // NTSC-U/C: 0x001700f0, PAL: 0x00172a00
    virtual void restoreGuts([[maybe_unused]] IBStream &stream) {
    }

    // NTSC-U/C: 0x00170140, PAL: 0x00172a50
    // The factory the unit's static initialiser registers under identifier zero.
    static Sch::Command *NewCmd() {
        return nullptr;
    }

    // The word at 0x0067a3f4, which the image initialises to zero.
    static int sCmdID;
};

int SteadyFBCmd::sCmdID;

/**
 * Scheduler command that runs ForceFeedbackMgr::SyncMetronome().
 *
 * `Q237_GLOBAL_$N$ForceFeedbackMgr.cppXFKhgb19StartMetronomeFBCmd` in the RTTI, with its vtable at
 * `0x007d9120`. The destructor at `0x00170148` is implicitly declared.
 */
class StartMetronomeFBCmd : public Sch::Command {
public:
    // NTSC-U/C: 0x001701e8, PAL: 0x00172af8
    virtual int CmdID() {
        return sCmdID;
    }

    // NTSC-U/C: 0x001701c0, PAL: 0x00172ad0
    virtual void Execute() {
        g_pForceFeedbackMgr->SyncMetronome();
    }

    // NTSC-U/C: 0x00170208, PAL: 0x00172b18
    virtual void Print(std::ostream &stream) {
        stream << "{" << "StartMetronomeFBCmd" << "}";
    }

    // NTSC-U/C: 0x001701f8, PAL: 0x00172b08
    virtual void saveGuts([[maybe_unused]] OBStream &stream) const {
    }

    // NTSC-U/C: 0x00170200, PAL: 0x00172b10
    virtual void restoreGuts([[maybe_unused]] IBStream &stream) {
    }

    // NTSC-U/C: 0x00170250, PAL: 0x00172b60
    // The factory the unit's static initialiser registers under identifier zero.
    static Sch::Command *NewCmd() {
        return nullptr;
    }

    // The word at 0x0067a3fc, which the image initialises to zero.
    static int sCmdID;
};

int StartMetronomeFBCmd::sCmdID;

/**
 * Scheduler command that ends a powerup effect on one slot.
 *
 * `Q237_GLOBAL_$N$ForceFeedbackMgr.cppXFKhgb15SetPowerupFBCmd` in the RTTI, with its vtable at
 * `0x007d90d8`. The destructor at `0x00170258` is implicitly declared.
 */
class SetPowerupFBCmd : public Sch::Command {
public:
    SetPowerupFBCmd(int nPlayerSlot, int bPowerup) : mPlayerSlot(nPlayerSlot), mPowerup(bPowerup) {
    }

    // NTSC-U/C: 0x001702d0, PAL: 0x00172be0
    virtual int CmdID() {
        return sCmdID;
    }

    // NTSC-U/C: 0x001702e0, PAL: 0x00172bf0
    virtual void Execute() {
        g_pForceFeedbackMgr->SetPowerup(mPlayerSlot, mPowerup);
    }

    // NTSC-U/C: 0x00170310, PAL: 0x00172c20
    // The factory the unit's static initialiser registers under identifier zero.
    static Sch::Command *NewCmd() {
        return nullptr;
    }

    // The word at 0x0067a404, which the image initialises to zero.
    static int sCmdID;

private:
    int mPlayerSlot; // +0x0c
    int mPowerup;    // +0x10
};

int SetPowerupFBCmd::sCmdID;

/**
 * Scheduler command that sets the small motor of one slot.
 *
 * `Q237_GLOBAL_$N$ForceFeedbackMgr.cppXFKhgb16SetSmallMotorCmd` in the RTTI, with its vtable at
 * `0x007d9098`. The destructor at `0x00170318` is implicitly declared.
 */
class SetSmallMotorCmd : public Sch::Command {
public:
    SetSmallMotorCmd(int nPlayerSlot, int nState) : mPlayerSlot(nPlayerSlot), mState(nState) {
    }

    // NTSC-U/C: 0x00170390, PAL: 0x00172ca0
    virtual int CmdID() {
        return sCmdID;
    }

    // NTSC-U/C: 0x001703a0, PAL: 0x00172cb0
    virtual void Execute() {
        g_pForceFeedbackMgr->SetSmallMotor(mPlayerSlot, mState);
    }

    // NTSC-U/C: 0x001703d0, PAL: 0x00172ce0
    // The factory the unit's static initialiser registers under identifier zero.
    static Sch::Command *NewCmd() {
        return nullptr;
    }

    // The word at 0x0067a40c, which the image initialises to zero.
    static int sCmdID;

private:
    int mPlayerSlot; // +0x0c
    int mState;      // +0x10
};

int SetSmallMotorCmd::sCmdID;

/**
 * Scheduler command for the big motor of one slot, never constructed.
 *
 * `Q237_GLOBAL_$N$ForceFeedbackMgr.cppXFKhgb16SetLargeMotorCmd` at `0x007d9478` is the only other
 * trace. The binary emits no vtable, CmdID(), or Execute() for the class.
 */
class SetLargeMotorCmd : public Sch::Command {
public:
    // NTSC-U/C: 0x001703d8, PAL: 0x00172ce8
    // The factory the unit's static initialiser registers under identifier zero.
    static Sch::Command *NewCmd() {
        return nullptr;
    }

    // The word at 0x0067a414, which no emitted routine reads.
    static int sCmdID;
};

[[maybe_unused]] int SetLargeMotorCmd::sCmdID;

/**
 * Scheduler command that sets both motors of one slot.
 *
 * `Q237_GLOBAL_$N$ForceFeedbackMgr.cppXFKhgb16SetBothMotorsCmd` in the RTTI, with its vtable at
 * `0x007d9048`. The destructor at `0x001703e0` is implicitly declared.
 */
class SetBothMotorsCmd : public Sch::Command {
public:
    SetBothMotorsCmd(int nPlayerSlot, int nSmallState, int nBigLevel)
        : mPlayerSlot(nPlayerSlot), mSmallState(nSmallState), mBigLevel(nBigLevel) {
    }

    // NTSC-U/C: 0x00170458, PAL: 0x00172d68
    virtual int CmdID() {
        return sCmdID;
    }

    // NTSC-U/C: 0x00170468, PAL: 0x00172d78
    virtual void Execute() {
        g_pForceFeedbackMgr->SetBothMotors(mPlayerSlot, mSmallState, mBigLevel);
    }

    // NTSC-U/C: 0x00170498, PAL: 0x00172da8
    // The factory the unit's static initialiser registers under identifier zero.
    static Sch::Command *NewCmd() {
        return nullptr;
    }

    // The word at 0x0067a41c, which the image initialises to zero.
    static int sCmdID;

private:
    int mPlayerSlot; // +0x0c
    int mSmallState; // +0x10
    int mBigLevel;   // +0x14
};

int SetBothMotorsCmd::sCmdID;

// Posts a command at a song position and gives the caller's reference back.
inline void PostAt(Sch::TickClock *pClock, Sch::Command *pCommand, int nTick) {
    pClock->PostAtSongTick(pCommand, nTick);
    if (pCommand != nullptr) {
        pCommand->Release();
    }
}

} // namespace

ForceFeedbackMgr::ForceFeedbackMgr() : mFlags(0), mUnusedTime(0), mPulseLength{0} {
    LoadConfig();
    g_pForceFeedbackMgr = this;
}

ForceFeedbackMgr::~ForceFeedbackMgr() {
    mEffects.clear();
    g_pForceFeedbackMgr = nullptr;
}

void ForceFeedbackMgr::LoadConfig() {
    std::vector<int> values;
    QueryConfigVector(&values, kMetronomeQuery);
    mMetronomeFirstSetting = values[0];
    if (kBarTicks / values[2] < values[1]) {
        values[1] = values[2];
    }
    mPulseLength.mValue = static_cast<long long>(values[1]) * kNanosecondsPerMillisecond;
    mBeatPeriod = Sch::Tick(kBarTicks / values[2]);

    const int aEffectQueries[] = {1204, 1205, 1202, 1203, 1206};
    mEffects.resize(kEffectCount, Effect());
    for (int i = 0; i < kEffectCount; ++i) {
        values.clear();
        QueryConfigVector(&values, aEffectQueries[i]);
        Effect effect;
        effect.mSmallMotor = values[0];
        effect.mBigMotor = values[1];
        effect.mPeriod = Sch::Tick(values[2]);
        effect.mPulseCount = values[3];
        mEffects[i] = effect;
    }
    mSlots.clear();
}

void ForceFeedbackMgr::StartMetronome(const Sch::Tick &delay) {
    mFlags &= ~kFlagStopped;
    if (mFlags != 0 && mFlags != kFlagPaused) {
        return;
    }
    StartMetronomeFBCmd *pCommand = new StartMetronomeFBCmd;
    const Sch::Tick when = MakePosition(SongClock()->SongTick() + delay.mTick);
    PostAt(SongClock(), pCommand, when.mTick);
}

void ForceFeedbackMgr::SetPlayerCount(unsigned int nPlayers) {
    mSlots.resize(nPlayers, Slot());
    if (static_cast<int>(nPlayers) <= kMaxVibratingPlayers) { // The binary tests the count signed.
        mFlags &= ~kFlagTooManyPlayers;
        return;
    }
    for (unsigned int i = 0; i < mSlots.size(); ++i) {
        SetBothMotors(i, kMotorOff, kMotorOff);
    }
    mFlags |= kFlagTooManyPlayers;
}

void ForceFeedbackMgr::PulseBeat() {
    if (mFlags == 0) {
        for (unsigned int i = 0; i < mSlots.size(); ++i) {
            if (mSlots[i].mPowerup != 0) {
                continue;
            }
            SetSmallMotor(i, kSmallMotorOn);
            SetSmallMotorCmd *pOff = new SetSmallMotorCmd(i, kMotorOff);
            SongClock()->PostIn(pOff, mPulseLength);
            if (pOff != nullptr) {
                pOff->Release();
            }
        }
    }
    if (mFlags != 0 && mFlags != kFlagPaused) {
        return;
    }

    SteadyFBCmd *pNext = new SteadyFBCmd;
    Sch::TickClock *pClock = SongClock();
    const Sch::Tick when = MakePosition(SongClock()->SongTick() + mBeatPeriod.mTick);
    PostAt(pClock, pNext, when.mTick);
}

void ForceFeedbackMgr::SyncMetronome() {
    Sch::TempoMap *pTempo = SongClock()->mTempoMap;
    const int nNow = SongClock()->SongTick();
    Sch::Tick toBeat(kBeatTicks - (nNow % kBeatTicks));

    const int nPulseMs =
        static_cast<int>((mPulseLength.mValue + kHalfMillisecondNs) / kNanosecondsPerMillisecond);
    const int nLeadNs = (nPulseMs / 2) + kMotorLeadNs;
    const Sch::Tick lead(
        static_cast<int>((nLeadNs + pTempo->mCeilingBias) / pTempo->mNanosecondsPerTick));
    if (toBeat.mTick < lead.mTick) {
        toBeat = MakePosition(toBeat.mTick + Sch::Tick(kBeatTicks).mTick);
    }

    SteadyFBCmd *pCommand = new SteadyFBCmd;
    Sch::TickClock *pClock = SongClock();
    const Sch::Tick beat = MakePosition(nNow + toBeat.mTick);
    const Sch::Tick when = MakePosition(beat.mTick - lead.mTick);
    PostAt(pClock, pCommand, when.mTick);
}

void ForceFeedbackMgr::PlayEffect(int nPlayerSlot, int nEffect) {
    if (mFlags != 0 || nPlayerSlot == kNoPlayerSlot) {
        return;
    }
    mSlots[nPlayerSlot].mPowerup = kPowerupRunning;
    const int nNow = SongClock()->SongTick();

    for (int i = 0; i < mEffects[nEffect].mPulseCount; ++i) {
        const Effect &effect = mEffects[nEffect];
        SetBothMotorsCmd *pOn =
            new SetBothMotorsCmd(nPlayerSlot, effect.mSmallMotor, effect.mBigMotor);
        Sch::TickClock *pClock = SongClock();
        const Sch::Tick spacing = MakePosition(effect.mPeriod.mTick * 2);
        const Sch::Tick offset = MakePosition(spacing.mTick * i);
        const Sch::Tick onWhen = MakePosition(nNow + offset.mTick);
        PostAt(pClock, pOn, onWhen.mTick);

        SetBothMotorsCmd *pOff = new SetBothMotorsCmd(nPlayerSlot, kMotorOff, kMotorOff);
        pClock = SongClock();
        // Every off command lands one period after the start. That is what the binary computes.
        const Sch::Tick offWhen = MakePosition(nNow + mEffects[nEffect].mPeriod.mTick);
        PostAt(pClock, pOff, offWhen.mTick);
    }

    SetPowerupFBCmd *pDone = new SetPowerupFBCmd(nPlayerSlot, kPowerupDone);
    Sch::TickClock *pClock = SongClock();
    const Sch::Tick doneWhen = MakePosition(nNow + mEffects[nEffect].mPeriod.mTick);
    PostAt(pClock, pDone, doneWhen.mTick);
}

void ForceFeedbackMgr::Suspend(unsigned char nMask) {
    for (unsigned int i = 0; i < mSlots.size(); ++i) {
        SetBothMotors(i, kMotorOff, kMotorOff);
    }
    mFlags |= nMask;
}

void ForceFeedbackMgr::SetPaused(int bPaused) {
    if (!bPaused) {
        mFlags &= ~kFlagPaused;
        return;
    }
    for (unsigned int i = 0; i < mSlots.size(); ++i) {
        SetBothMotors(i, kMotorOff, kMotorOff);
    }
    mFlags |= kFlagPaused;
}

void ForceFeedbackMgr::SetJukeboxMode(int bJukebox) {
    if (!bJukebox) {
        mFlags &= ~kFlagJukebox;
        return;
    }
    for (unsigned int i = 0; i < mSlots.size(); ++i) {
        SetBothMotors(i, kMotorOff, kMotorOff);
    }
    mFlags |= kFlagJukebox;
}

void ForceFeedbackMgr::SetPlaybackMode(int bPlayback) {
    if (!bPlayback) {
        mFlags &= ~kFlagPlayback;
        return;
    }
    for (unsigned int i = 0; i < mSlots.size(); ++i) {
        SetBothMotors(i, kMotorOff, kMotorOff);
    }
    mFlags |= kFlagPlayback;
}

void ForceFeedbackMgr::SetEnabled(int bEnabled) {
    if (bEnabled) {
        mFlags &= ~kFlagDisabled;
        return;
    }
    for (unsigned int i = 0; i < mSlots.size(); ++i) {
        SetBothMotors(i, kMotorOff, kMotorOff);
    }
    mFlags |= kFlagDisabled;
}

void ForceFeedbackMgr::SetPowerup(unsigned int nPlayerSlot, int bPowerup) {
    if (nPlayerSlot < mSlots.size()) {
        mSlots[nPlayerSlot].mPowerup = bPowerup;
    }
}

void ForceFeedbackMgr::StopAll([[maybe_unused]] Sch::Tick when) {
    for (unsigned int i = 0; i < mSlots.size(); ++i) {
        SetBothMotors(i, kMotorOff, kMotorOff);
    }
    // The same loop runs a second time. That is what the binary does.
    for (unsigned int i = 0; i < mSlots.size(); ++i) {
        SetBothMotors(i, kMotorOff, kMotorOff);
    }
    mFlags |= kFlagStopped;
}

void ForceFeedbackMgr::SetBigMotor(int nPlayerSlot, int nLevel) {
    if (mFlags != 0) {
        return;
    }
    mSlots[nPlayerSlot].mBigMotor = nLevel;
    ApplyMotors(nPlayerSlot);
}

void ForceFeedbackMgr::SetSmallMotor(int nPlayerSlot, int nState) {
    if (mFlags != 0) {
        return;
    }
    mSlots[nPlayerSlot].mSmallMotor = nState;
    ApplyMotors(nPlayerSlot);
}

void ForceFeedbackMgr::SetBothMotors(int nPlayerSlot, int nSmallState, int nBigLevel) {
    if (mFlags != 0) {
        return;
    }
    mSlots[nPlayerSlot].mSmallMotor = nSmallState;
    mSlots[nPlayerSlot].mBigMotor = nBigLevel;
    ApplyMotors(nPlayerSlot);
}

void ForceFeedbackMgr::ApplyMotors(int nPlayerSlot) {
    InputPoller *pPoller = Application::shared()->GetGameManager()->GetPoller();
    const Slot &slot = mSlots[nPlayerSlot];
    pPoller->SetVibration(nPlayerSlot + 1, slot.mSmallMotor, slot.mBigMotor);
}

void ForceFeedbackMgr::PlayUnusedEffect(Player *pPlayer) {
    PlayEffect(pPlayer->GetInputSlot(), kEffectUnused);
}

void ForceFeedbackMgr::PlayBumpEffect(Player *pPlayer) {
    PlayEffect(pPlayer->GetInputSlot(), kEffectBump);
}

void ForceFeedbackMgr::PlayAutocatchEffect(Player *pPlayer) {
    PlayEffect(pPlayer->GetInputSlot(), kEffectAutocatch);
}

void ForceFeedbackMgr::PlayNeutralizedEffect(Player *pPlayer) {
    PlayEffect(pPlayer->GetInputSlot(), kEffectNeutralized);
}

void ForceFeedbackMgr::PlayCrippleEffect(Player *pPlayer) {
    PlayEffect(pPlayer->GetInputSlot(), kEffectCripple);
}
