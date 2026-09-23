#pragma once

class Player;

/**
 * Driver of the controllers' vibration motors during a song.
 *
 * The class is not polymorphic and emits no RTTI. The name is inferred from its translation unit.
 * The five file-local scheduler commands beside it record `ForceFeedbackMgr.cpp` in their
 * anonymous-namespace RTTI names (SteadyFBCmd, StartMetronomeFBCmd, SetPowerupFBCmd,
 * SetSmallMotorCmd, and SetBothMotorsCmd). The object is 0x48 bytes. GrooveWorld builds one
 * through the constructor at `0x0016dae0`, keeps it at `+0x34`, and deletes it through the
 * destructor at `0x0016dca8`.
 *
 * Only the surface other classes call is declared. The routines between `0x00170a68` and
 * `0x00170c40` are one-line wrappers that pass a player's slot and a fixed effect number to
 * PlayEffect(), each for one game event. The members are not recovered.
 */
class ForceFeedbackMgr {
public:
    /**
     * Start one vibration effect on a player's controller.
     *
     * The body is not written. It does nothing while the byte at `+0x00` is set or the slot is -1,
     * marks the slot's twelve-byte record in the table at `+0x04`, reads the song clock, and
     * schedules the effect's motor commands.
     *
     * @param nPlayerSlot The player's slot, as Player::Slot2() reports it.
     * @param nEffect The effect number.
     * @ghidraAddress 0x0016e848
     */
    void PlayEffect(int nPlayerSlot, int nEffect);

    /**
     * Play the effect a crippler hit produces on a player's controller.
     *
     * The body is not written. It calls PlayEffect() with Player::Slot2() of the player and
     * effect 2. TnlCrippleFX's frame routine at `0x0043e500` is the recovered caller.
     *
     * @param pPlayer The player that was hit.
     * @ghidraAddress 0x00170c40
     */
    void PlayCrippleEffect(Player *pPlayer);
};
