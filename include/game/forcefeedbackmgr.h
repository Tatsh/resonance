#pragma once

#include <vector>

#include "mid/mbt.h"
#include "sch/tick.h"

class Player;

/**
 * Driver of the controllers' vibration motors during a song.
 *
 * The class is not polymorphic and emits no RTTI. The name is inferred from its translation unit.
 * The five file-local scheduler commands beside it record `ForceFeedbackMgr.cpp` in their
 * anonymous-namespace RTTI names (SteadyFBCmd, StartMetronomeFBCmd, SetPowerupFBCmd,
 * SetSmallMotorCmd, and SetBothMotorsCmd). The object is 0x48 bytes. GrooveWorld builds one, keeps
 * it at `+0x34`, and deletes it in GrooveWorld::Shutdown(). The constructor also stores the object
 * in the unit's pointer at `0x0067a3f0`, which every command's Execute() goes through.
 *
 * Two kinds of vibration run through it. The metronome pulses the small motor of every player not
 * inside a powerup effect once a beat, and an effect runs one of five configured motor patterns on
 * one player's controller. Any bit set in mFlags suspends both.
 *
 * The unit's static initialiser at `0x0016ff68` hands six factory functions to the command
 * registrar at `0x00538208` with an identifier of zero, and each factory returns null. Five are the
 * commands' NewCmd() members. The sixth, at `0x001703d8`, belongs to no recovered class.
 */
class ForceFeedbackMgr {
public:
    /** One controller's motor state. */
    struct Slot {
        int mPowerup;    /*!< Non-zero while an effect runs. The metronome skips the slot. */
        int mBigMotor;   /*!< The big motor's level. */
        int mSmallMotor; /*!< The small motor's state, 0 or 1. */
    };

    /** One vibration effect, read from the configuration by LoadConfig(). */
    struct Effect {
        int mSmallMotor;  /*!< The small motor's state while a pulse is on. */
        int mBigMotor;    /*!< The big motor's level while a pulse is on. */
        int mPulseCount;  /*!< The pulses the effect runs. */
        Mid::MBT mPeriod; /*!< The length of one pulse, in MIDI ticks. */
    };

    /**
     * Load the configuration and register the object as the unit's instance.
     *
     * @ghidraAddress 0x0016dae0
     */
    ForceFeedbackMgr();

    /**
     * Clear mEffects and the unit's instance pointer.
     *
     * @ghidraAddress 0x0016dca8
     */
    ~ForceFeedbackMgr();

    /**
     * Schedule the metronome's start a delay from the current song position.
     *
     * The stopped flag is cleared first. Nothing is scheduled unless the remaining flags are clear
     * or the paused flag alone is set.
     *
     * @param delay The delay, in MIDI ticks.
     * @ghidraAddress 0x0016e1b8
     */
    void StartMetronome(const Mid::MBT &delay);

    /**
     * Size mSlots to the players and suspend everything for three players or more.
     *
     * @param nPlayers The player count.
     * @ghidraAddress 0x0016e2b8
     */
    void SetPlayerCount(unsigned int nPlayers);

    /**
     * Pulse the small motor of every idle controller for mPulseLength and schedule the next beat.
     *
     * The file-local SteadyFBCmd runs it.
     *
     * @ghidraAddress 0x0016e408
     */
    void PulseBeat();

    /**
     * Schedule the first beat pulse ahead of the next beat by the motors' latency.
     *
     * The file-local StartMetronomeFBCmd runs it. The lead is half the pulse length in
     * milliseconds plus 90000000, converted to MIDI ticks through the song clock's tempo map. A
     * beat closer than the lead moves the pulse one beat on.
     *
     * @ghidraAddress 0x0016e610
     */
    void SyncMetronome();

    /**
     * Start one vibration effect on a player's controller.
     *
     * Nothing happens while any flag is set or the slot is -1. The slot is marked as running an
     * effect, each pulse turns both motors on at twice the period times the pulse index and off
     * again, and a closing SetPowerupFBCmd clears the mark after one period.
     *
     * @param nPlayerSlot The player's slot, as Player::Slot2() reports it.
     * @param nEffect The effect number, an index into mEffects.
     * @ghidraAddress 0x0016e848
     */
    void PlayEffect(int nPlayerSlot, int nEffect);

    /**
     * Stop every motor and set flags in mFlags. The image has no caller.
     *
     * @param nMask The flags to set.
     * @ghidraAddress 0x001704c8
     */
    void Suspend(unsigned char nMask);

    /**
     * Stop every motor and set the paused flag, or clear the flag.
     *
     * GameManagerImpl's pause and unpause handlers are the recovered callers.
     *
     * @param bPaused Non-zero to pause.
     * @ghidraAddress 0x00170588
     */
    void SetPaused(int bPaused);

    /**
     * Stop every motor and set the jukebox flag, or clear the flag.
     *
     * GrooveWorld passes Globals::IsJukeboxMode().
     *
     * @param bJukebox Non-zero in jukebox mode.
     * @ghidraAddress 0x00170648
     */
    void SetJukeboxMode(int bJukebox);

    /**
     * Stop every motor and set the flag at bit 2, or clear the flag.
     *
     * GrooveWorld passes its word at `+0x8c`.
     *
     * @param bSet Non-zero to set the flag.
     * @ghidraAddress 0x00170708
     */
    void SetUnknownFlag04(int bSet);

    /**
     * Clear the disabled flag, or stop every motor and set it.
     *
     * @param bEnabled Non-zero to allow vibration.
     * @ghidraAddress 0x001707c8
     */
    void SetEnabled(int bEnabled);

    /**
     * Set a slot's powerup mark when the slot exists. SetPowerupFBCmd runs it.
     *
     * @param nPlayerSlot The slot.
     * @param bPowerup The mark.
     * @ghidraAddress 0x00170890
     */
    void SetPowerup(unsigned int nPlayerSlot, int bPowerup);

    /**
     * Stop every motor twice over and set the stopped flag. The image has no caller.
     *
     * @ghidraAddress 0x001708d0
     */
    void StopAll();

    /**
     * @param nPlayerSlot The slot.
     * @param nLevel The big motor's level.
     * @ghidraAddress 0x001709f8
     */
    void SetBigMotor(int nPlayerSlot, int nLevel);

    /**
     * @param nPlayerSlot The slot.
     * @param nState The small motor's state.
     * @ghidraAddress 0x00170a30
     */
    void SetSmallMotor(int nPlayerSlot, int nState);

    /**
     * @param nPlayerSlot The slot.
     * @param nSmallState The small motor's state.
     * @param nBigLevel The big motor's level.
     * @ghidraAddress 0x00170a68
     */
    void SetBothMotors(int nPlayerSlot, int nSmallState, int nBigLevel);

    /**
     * Pass a slot's motor state to the controller through InputPoller::SetVibration().
     *
     * @param nPlayerSlot The slot. The controller's port is one more.
     * @ghidraAddress 0x00170ab0
     */
    void ApplyMotors(int nPlayerSlot);

    /**
     * Play effect 4 on a player's controller. The image has no caller.
     *
     * @param pPlayer The player.
     * @ghidraAddress 0x00170b20
     */
    void PlayEffect4(Player *pPlayer);

    /**
     * Play effect 1 on a player's controller. The routine at `0x00448d58` calls it.
     *
     * @param pPlayer The player.
     * @ghidraAddress 0x00170b68
     */
    void PlayEffect1(Player *pPlayer);

    /**
     * Play effect 0 on a player's controller. The routine at `0x00448d58` calls it.
     *
     * @param pPlayer The player.
     * @ghidraAddress 0x00170bb0
     */
    void PlayEffect0(Player *pPlayer);

    /**
     * Play effect 3 on a player's controller. AppTunnel::HandleMessage() calls it.
     *
     * @param pPlayer The player.
     * @ghidraAddress 0x00170bf8
     */
    void PlayEffect3(Player *pPlayer);

    /**
     * Play the effect a crippler hit produces, effect 2, on a player's controller.
     *
     * TnlCrippleFX's frame routine at `0x0043e500` is the recovered caller.
     *
     * @param pPlayer The player that was hit.
     * @ghidraAddress 0x00170c40
     */
    void PlayCrippleEffect(Player *pPlayer);

private:
    // Reads the metronome settings (configuration 0x4b1) and the five effects (0x4b4, 0x4b5,
    // 0x4b2, 0x4b3, and 0x4b6), and empties mSlots.
    // 0x0016de58
    void LoadConfig();

    unsigned char mFlags;         // +0x00, any set bit suspends vibration
    std::vector<Slot> mSlots;     // +0x04
    std::vector<Effect> mEffects; // +0x10
    int mUnknown1c;               // +0x1c, not written by the constructor
    int mUnknown20;               // +0x20, not written by the constructor
    Mid::MBT mUnknown24;          // +0x24
    long long mUnknown28;         // +0x28, zero on construction
    int mUnknown30;               // +0x30, the first metronome setting
    Sch::Tick mPulseLength;       // +0x38, in nanoseconds
    Mid::MBT mBeatPeriod;         // +0x40, the bar divided by the third metronome setting
};
