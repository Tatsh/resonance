#pragma once

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
 * The thirteen subclasses themselves are not reconstructed here. Seven have their own RTTI
 * descriptor, AutocatchPowerup, BumpPowerup, CripplePowerup, EffectPowerup, FreestylePowerup,
 * GhostNotesPowerup, MultiplierPowerup, and NeutralizePowerup among them, and the factory's jump
 * table at `0x007e3f70` sends six of its thirteen cases to one shared arm.
 */
class Powerup {
public:
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
     * @return Non-zero once the effect has been applied.
     */
    virtual int Deploy() = 0;

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
     * `Powerup` tag and installs one subclass table. A kind of 13 or above returns a null pointer,
     * and the two collections both store the result with no test.
     *
     * The body is not written yet, because the thirteen subclasses have no headers.
     *
     * @param nType The kind.
     * @return The powerup, or a null pointer for a kind of 13 or above.
     * @ghidraAddress 0x001c65f0
     */
    static Powerup *CreateForType(int nType);
};
