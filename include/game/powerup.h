#pragma once

#include <cstddef>

#include "os/mem.h"

class Player;

/**
 * Base of the thirteen powerups a player can store and deploy.
 *
 * `7Powerup` in the RTTI descriptor at `0x0086f690`, a leaf with no base list. The object is four
 * bytes of vtable pointer and the class has no data member, which the factory confirms by
 * allocating four bytes for the subclasses that add nothing. Its table at `0x007e4690` has four
 * entries with a zero terminator at index 4, and slots 2 and 3 both address the shared
 * pure-virtual stub at `0x005381a8`, so the class is abstract.
 *
 * The two method names are inferred from the bodies of the subclasses and of the two collections
 * that use them. No method name survives anywhere in the image.
 *
 * Eight subclasses implement the thirteen kinds, which are the HudItemKind values. Seven kinds
 * each have their own class (NeutralizePowerup, CripplePowerup, FreestylePowerup,
 * AutocatchPowerup, BumpPowerup, GhostNotesPowerup, and MultiplierPowerup), and the factory's jump
 * table at `0x007e3f70` sends the six effect kinds to one shared arm that builds an EffectPowerup
 * recording its kind.
 */
class Powerup {
public:
    /**
     * Allocate a powerup from the tagged heap under the tag `Powerup`.
     *
     * No out-of-line body exists. CreateForType() expands the call in every arm.
     *
     * @param nSize The object size the compiler supplies.
     * @return The block.
     */
    void *operator new(size_t nSize) {
        return AllocateTaggedMemory(nSize, "Powerup");
    }

    /**
     * Release a powerup to the tagged heap.
     *
     * No out-of-line body exists. Every deleting destructor expands the call.
     *
     * @param pBlock The block.
     */
    void operator delete(void *pBlock) {
        FreeTaggedMemory(pBlock, "Powerup");
    }

    /**
     * @ghidraAddress 0x001ca4c8
     */
    virtual ~Powerup();

    /**
     * Apply the powerup's effect.
     *
     * Slot 2, and pure. Both collections call it and proceed only when it reports success, which
     * is what recovers the return value as a result rather than as a state read. NeutralizePowerup
     * implements it at `0x001c9c40` and CripplePowerup at `0x001c9830`.
     *
     * The arity comes from the two call sites (`0x001cb538` and `0x001cb97c`), which forward the
     * collection's two arguments untouched in a1 and a2, load the owning player into a3, and clear
     * t0. NeutralizePowerup stores a1 where PhraseNeutralizer compares the track and a2 where it
     * reads the bar. No implementation reads the fourth argument.
     *
     * @param nTrack The deploying player's track, from Player::Slot4().
     * @param nBar The current bar.
     * @param pPlayer The deploying player.
     * @param nUnused Zero at both call sites.
     * @return Non-zero once the effect has been applied.
     */
    virtual int Deploy(int nTrack, int nBar, Player *pPlayer, int nUnused) = 0;

    /**
     * Report which of the thirteen kinds this powerup is.
     *
     * Slot 3, and pure. Every implementation is a two-instruction body that returns a constant,
     * and the constant is the index the factory accepts, which PowerupCollection::AddPowerup()
     * confirms by matching an incoming kind against it.
     *
     * @return The kind.
     */
    virtual int Type() = 0;

    /**
     * Produce one powerup of the requested kind on the heap.
     *
     * The body is a jump table over thirteen kinds at `0x007e3f70`. Each arm allocates against the
     * `Powerup` tag and installs one subclass table. A kind outside 0 through 12 (the unsigned
     * comparison rejects a negative kind too) returns a null pointer, and the two collections
     * both store the result with no test.
     *
     * @param nType The kind, a HudItemKind.
     * @return The powerup, or a null pointer for a kind outside 0 through 12.
     * @ghidraAddress 0x001c65f0
     */
    static Powerup *CreateForType(int nType);
};
