#pragma once

class MusePlayer;

/**
 * Owner a MusePlayer reports back to.
 *
 * `10MuseParent` in the RTTI descriptor at `0x0086f798`, with no base list. The class declares no
 * data member and no virtual destructor, so its subobject is the compiler-generated vptr alone and
 * is four bytes. MuseSynth places it at `+0x04` and is the one implementation recovered.
 *
 * The table has three entries. Slot 0 is the type function and the two that follow are the
 * class's own virtuals. There is no destructor slot, which is what proves the class declares none.
 *
 * Both members take the reporting player. Their meaning comes from MuseSynth's two bodies and from
 * MultiMusePlayer's two overrides, which forward each one further up the chain.
 */
class MuseParent {
public:
    /**
     * Retain only the reporting player.
     *
     * Table slot 1. MuseSynth's body at `0x001aa908` performs no work unless the player reports
     * non-zero from MusePlayer slot 4, and then deletes and unlinks every other player it owns.
     * MultiMusePlayer's override at `0x001aa1f8` reports the same request to its own parent once,
     * the first time, before running MuseSynth's body.
     *
     * @param pPlayer The player to retain.
     */
    virtual void RetainOnly(MusePlayer *pPlayer) = 0;

    /**
     * Report that a player has finished.
     *
     * Table slot 2. MuseSynth's body at `0x001aa9f0` unlinks the player and deletes it.
     * MultiMusePlayer's override at `0x001a9c30` additionally reports itself finished to its own
     * parent once its schedule is exhausted and the last player is gone.
     *
     * @param pPlayer The player that has finished.
     */
    virtual void PlayerFinished(MusePlayer *pPlayer) = 0;
};
