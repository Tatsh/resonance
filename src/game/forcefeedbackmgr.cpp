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
constexpr unsigned char kFlagUnknown04 = 0x04;
constexpr unsigned char kFlagPaused = 0x08;
constexpr unsigned char kFlagStopped = 0x10;
constexpr unsigned char kFlagDisabled = 0x20;

// SetPlayerCount() suspends vibration from this many players up.
constexpr unsigned int kMaxVibratingPlayers = 2;

// PlayEffect() ignores a player without a slot.
constexpr int kNoPlayerSlot = -1;

// One bar and one beat at 480 ticks per quarter note.
constexpr int kBarTicks = 1920;
constexpr int kBeatTicks = 480;

// The configuration codes LoadConfig() reads.
constexpr int kMetronomeQuery = 0x4b1;
constexpr int kEffectCount = 5;

// SyncMetronome() rounds the pulse to milliseconds and adds this lead before converting to ticks.
constexpr long long kNanosecondsPerMillisecond = 1000000;
constexpr long long kHalfMillisecondNs = 500000;
constexpr int kMotorLeadNs = 90000000;

// The effect numbers the one-line wrappers pass.
constexpr int kEffect0 = 0;
constexpr int kEffect1 = 1;
constexpr int kEffectCripple = 2;
constexpr int kEffect3 = 3;
constexpr int kEffect4 = 4;

// Motor states.
constexpr int kMotorOff = 0;
constexpr int kSmallMotorOn = 1;
constexpr int kPowerupRunning = 1;
constexpr int kPowerupDone = 0;

// 0x0067a3f0. The instance the commands run against, set by the constructor.
ForceFeedbackMgr *g_pForceFeedbackMgr;

// The clamp the inline Mid::MBT arithmetic applies to a computed position.
inline Mid::MBT MakePosition(int nTick) {
    return Mid::MBT(std::min(std::max(nTick, kMBTMinimum), kMBTMaximum));
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
    // 0x001700d8
    virtual int CmdID() {
        return sCmdID;
    }

    // 0x001700b0
    virtual void Execute() {
        g_pForceFeedbackMgr->PulseBeat();
    }

    // 0x001700f8
    virtual void Print(std::ostream &stream) {
        stream << "{" << "SteadyFBCmd" << "}";
    }

    // 0x001700e8
    virtual void Save([[maybe_unused]] OBStream &stream) {
    }

    // 0x001700f0
    virtual void Load([[maybe_unused]] IBStream &stream) {
    }

    // 0x00170140
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
    // 0x001701e8
    virtual int CmdID() {
        return sCmdID;
    }

    // 0x001701c0
    virtual void Execute() {
        g_pForceFeedbackMgr->SyncMetronome();
    }

    // 0x00170208
    virtual void Print(std::ostream &stream) {
        stream << "{" << "StartMetronomeFBCmd" << "}";
    }

    // 0x001701f8
    virtual void Save([[maybe_unused]] OBStream &stream) {
    }

    // 0x00170200
    virtual void Load([[maybe_unused]] IBStream &stream) {
    }

    // 0x00170250
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

    // 0x001702d0
    virtual int CmdID() {
        return sCmdID;
    }

    // 0x001702e0
    virtual void Execute() {
        g_pForceFeedbackMgr->SetPowerup(mPlayerSlot, mPowerup);
    }

    // 0x00170310
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

    // 0x00170390
    virtual int CmdID() {
        return sCmdID;
    }

    // 0x001703a0
    virtual void Execute() {
        g_pForceFeedbackMgr->SetSmallMotor(mPlayerSlot, mState);
    }

    // 0x001703d0
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
    // 0x001703d8
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

    // 0x00170458
    virtual int CmdID() {
        return sCmdID;
    }

    // 0x00170468
    virtual void Execute() {
        g_pForceFeedbackMgr->SetBothMotors(mPlayerSlot, mSmallState, mBigLevel);
    }

    // 0x00170498
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

// 0x0016dae0
ForceFeedbackMgr::ForceFeedbackMgr() : mFlags(0), mUnknown28(0), mPulseLength{0} {
    LoadConfig();
    g_pForceFeedbackMgr = this;
}

// 0x0016dca8
ForceFeedbackMgr::~ForceFeedbackMgr() {
    mEffects.clear();
    g_pForceFeedbackMgr = nullptr;
}

// 0x0016de58
void ForceFeedbackMgr::LoadConfig() {
    std::vector<int> values;
    QueryConfigVector(&values, kMetronomeQuery);
    mUnknown30 = values[0];
    if (kBarTicks / values[2] < values[1]) {
        values[1] = values[2];
    }
    mPulseLength.mValue = static_cast<long long>(values[1]) * kNanosecondsPerMillisecond;
    mBeatPeriod = Mid::MBT(kBarTicks / values[2]);

    const int aEffectQueries[] = {0x4b4, 0x4b5, 0x4b2, 0x4b3, 0x4b6};
    mEffects.resize(kEffectCount, Effect());
    for (int i = 0; i < kEffectCount; ++i) {
        values.clear();
        QueryConfigVector(&values, aEffectQueries[i]);
        Effect effect;
        effect.mSmallMotor = values[0];
        effect.mBigMotor = values[1];
        effect.mPeriod = Mid::MBT(values[2]);
        effect.mPulseCount = values[3];
        mEffects[i] = effect;
    }
    mSlots.clear();
}

// 0x0016e1b8
void ForceFeedbackMgr::StartMetronome(const Mid::MBT &delay) {
    mFlags &= ~kFlagStopped;
    if (mFlags != 0 && mFlags != kFlagPaused) {
        return;
    }
    StartMetronomeFBCmd *pCommand = new StartMetronomeFBCmd;
    const Mid::MBT when = MakePosition(SongClock()->SongTick() + delay.mTick);
    PostAt(SongClock(), pCommand, when.mTick);
}

// 0x0016e2b8
void ForceFeedbackMgr::SetPlayerCount(unsigned int nPlayers) {
    mSlots.resize(nPlayers, Slot());
    if (nPlayers <= kMaxVibratingPlayers) {
        mFlags &= ~kFlagTooManyPlayers;
        return;
    }
    for (unsigned int i = 0; i < mSlots.size(); ++i) {
        SetBothMotors(i, kMotorOff, kMotorOff);
    }
    mFlags |= kFlagTooManyPlayers;
}

// 0x0016e408
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
    const Mid::MBT when = MakePosition(SongClock()->SongTick() + mBeatPeriod.mTick);
    PostAt(pClock, pNext, when.mTick);
}

// 0x0016e610
void ForceFeedbackMgr::SyncMetronome() {
    Sch::TempoMap *pTempo = SongClock()->mTempoMap;
    const int nNow = SongClock()->SongTick();
    Mid::MBT toBeat(kBeatTicks - (nNow % kBeatTicks));

    const int nPulseMs =
        static_cast<int>((mPulseLength.mValue + kHalfMillisecondNs) / kNanosecondsPerMillisecond);
    const int nLeadNs = (nPulseMs / 2) + kMotorLeadNs;
    const Mid::MBT lead(
        static_cast<int>((nLeadNs + pTempo->mCeilingBias) / pTempo->mNanosecondsPerTick));
    if (toBeat.mTick < lead.mTick) {
        toBeat = MakePosition(toBeat.mTick + Mid::MBT(kBeatTicks).mTick);
    }

    SteadyFBCmd *pCommand = new SteadyFBCmd;
    Sch::TickClock *pClock = SongClock();
    const Mid::MBT beat = MakePosition(nNow + toBeat.mTick);
    const Mid::MBT when = MakePosition(beat.mTick - lead.mTick);
    PostAt(pClock, pCommand, when.mTick);
}

// 0x0016e848
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
        const Mid::MBT spacing = MakePosition(effect.mPeriod.mTick * 2);
        const Mid::MBT offset = MakePosition(spacing.mTick * i);
        const Mid::MBT onWhen = MakePosition(nNow + offset.mTick);
        PostAt(pClock, pOn, onWhen.mTick);

        SetBothMotorsCmd *pOff = new SetBothMotorsCmd(nPlayerSlot, kMotorOff, kMotorOff);
        pClock = SongClock();
        // Every off command lands one period after the start. That is what the binary computes.
        const Mid::MBT offWhen = MakePosition(nNow + mEffects[nEffect].mPeriod.mTick);
        PostAt(pClock, pOff, offWhen.mTick);
    }

    SetPowerupFBCmd *pDone = new SetPowerupFBCmd(nPlayerSlot, kPowerupDone);
    Sch::TickClock *pClock = SongClock();
    const Mid::MBT doneWhen = MakePosition(nNow + mEffects[nEffect].mPeriod.mTick);
    PostAt(pClock, pDone, doneWhen.mTick);
}

// 0x001704c8
void ForceFeedbackMgr::Suspend(unsigned char nMask) {
    for (unsigned int i = 0; i < mSlots.size(); ++i) {
        SetBothMotors(i, kMotorOff, kMotorOff);
    }
    mFlags |= nMask;
}

// 0x00170588
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

// 0x00170648
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

// 0x00170708
void ForceFeedbackMgr::SetUnknownFlag04(int bSet) {
    if (!bSet) {
        mFlags &= ~kFlagUnknown04;
        return;
    }
    for (unsigned int i = 0; i < mSlots.size(); ++i) {
        SetBothMotors(i, kMotorOff, kMotorOff);
    }
    mFlags |= kFlagUnknown04;
}

// 0x001707c8
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

// 0x00170890
void ForceFeedbackMgr::SetPowerup(unsigned int nPlayerSlot, int bPowerup) {
    if (nPlayerSlot < mSlots.size()) {
        mSlots[nPlayerSlot].mPowerup = bPowerup;
    }
}

// 0x001708d0
void ForceFeedbackMgr::StopAll([[maybe_unused]] Mid::MBT when) {
    for (unsigned int i = 0; i < mSlots.size(); ++i) {
        SetBothMotors(i, kMotorOff, kMotorOff);
    }
    // The same loop runs a second time. That is what the binary does.
    for (unsigned int i = 0; i < mSlots.size(); ++i) {
        SetBothMotors(i, kMotorOff, kMotorOff);
    }
    mFlags |= kFlagStopped;
}

// 0x001709f8
void ForceFeedbackMgr::SetBigMotor(int nPlayerSlot, int nLevel) {
    if (mFlags != 0) {
        return;
    }
    mSlots[nPlayerSlot].mBigMotor = nLevel;
    ApplyMotors(nPlayerSlot);
}

// 0x00170a30
void ForceFeedbackMgr::SetSmallMotor(int nPlayerSlot, int nState) {
    if (mFlags != 0) {
        return;
    }
    mSlots[nPlayerSlot].mSmallMotor = nState;
    ApplyMotors(nPlayerSlot);
}

// 0x00170a68
void ForceFeedbackMgr::SetBothMotors(int nPlayerSlot, int nSmallState, int nBigLevel) {
    if (mFlags != 0) {
        return;
    }
    mSlots[nPlayerSlot].mSmallMotor = nSmallState;
    mSlots[nPlayerSlot].mBigMotor = nBigLevel;
    ApplyMotors(nPlayerSlot);
}

// 0x00170ab0
void ForceFeedbackMgr::ApplyMotors(int nPlayerSlot) {
    InputPoller *pPoller = Application::shared()->GetGameManager()->GetPoller();
    const Slot &slot = mSlots[nPlayerSlot];
    pPoller->SetVibration(nPlayerSlot + 1, slot.mSmallMotor, slot.mBigMotor);
}

// 0x00170b20
void ForceFeedbackMgr::PlayEffect4(Player *pPlayer) {
    PlayEffect(pPlayer->Slot2(), kEffect4);
}

// 0x00170b68
void ForceFeedbackMgr::PlayEffect1(Player *pPlayer) {
    PlayEffect(pPlayer->Slot2(), kEffect1);
}

// 0x00170bb0
void ForceFeedbackMgr::PlayEffect0(Player *pPlayer) {
    PlayEffect(pPlayer->Slot2(), kEffect0);
}

// 0x00170bf8
void ForceFeedbackMgr::PlayEffect3(Player *pPlayer) {
    PlayEffect(pPlayer->Slot2(), kEffect3);
}

// 0x00170c40
void ForceFeedbackMgr::PlayCrippleEffect(Player *pPlayer) {
    PlayEffect(pPlayer->Slot2(), kEffectCripple);
}
