#pragma once

#include "game/inputcheatdetector.h"

/**
 * Cheat detector for the front end.
 *
 * `21InputCheatDetectorMet` in the RTTI descriptor at `0x008efcb0`, with InputCheatDetector as its
 * one public base at offset 0. It adds no member, and MetaGameWorld allocates it at 8 bytes. Its
 * vtable at `0x00810ee8` has four entries and a zero terminator at index 4: the type function at
 * `0x003d4708`, the destructor at `0x003d4758`, InputCheatDetector's slot 2 at `0x001dc658`
 * inherited, and its own slot 3 at `0x003d4788`.
 *
 * The destructor at `0x003d4758` stores RawController's table and releases the object. It is
 * byte-identical to InputCheatDetector's at `0x001deb00`, a re-emission of the same inline chain in
 * the front-end translation unit, and is implicitly declared, so this class owes no definition.
 *
 * The constructor has no address. MetaGameWorld's constructor runs InputCheatDetector's at
 * `0x001daaf8` and then stores this class's table over the base table, the expansion of an inline
 * constructor that forwards its one argument.
 */
class InputCheatDetectorMet : public InputCheatDetector {
public:
    /**
     * Attach the detector to a table of cheats.
     *
     * Inline, with no address of its own. MetaGameWorld's constructor expands it at `0x003d3150`.
     *
     * @param pCheats The table to match against.
     */
    explicit InputCheatDetectorMet(std::vector<CheatSequence> *pCheats)
        : InputCheatDetector(pCheats) {
    }

    /**
     * Unrecovered. Slot 3, with an empty body.
     *
     * @ghidraAddress 0x003d4788
     */
    virtual void OnUnknownSlot3();
};
