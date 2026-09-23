#pragma once

#include "game/inputcheatdetector.h"

/**
 * Cheat detector for a game session.
 *
 * `20InputCheatDetectorGS` in the RTTI descriptor at `0x008ef5c0`, with InputCheatDetector as its
 * one public base at offset 0. It adds no member, and GrooveWorld allocates it at 8 bytes. Its
 * vtable at `0x007dc408` has four entries and a zero terminator at index 4: the type function at
 * `0x00194058`, the destructor at `0x001940a8`, InputCheatDetector's slot 2 at `0x001dc658`
 * inherited, and its own slot 3 at `0x001940d8`.
 *
 * The destructor at `0x001940a8` stores RawController's table and releases the object. It is the
 * implicitly declared destructor, re-emitted in GrooveWorld's translation unit, so this class owes
 * no definition.
 *
 * The constructor has no address. GrooveWorld's constructor runs InputCheatDetector's at
 * `0x001daaf8` and then stores this class's table over the base table at `0x0018c030`, the
 * expansion of an inline constructor that forwards its one argument.
 */
class InputCheatDetectorGS : public InputCheatDetector {
public:
    /**
     * Attach the detector to a table of cheats.
     *
     * Inline, with no address of its own. GrooveWorld's constructor expands it with
     * g_gameCheatSequences.
     *
     * @param pCheats The table to match against.
     */
    explicit InputCheatDetectorGS(std::vector<CheatSequence> *pCheats)
        : InputCheatDetector(pCheats) {
    }

    /**
     * Unrecovered. Slot 3, with an empty body.
     *
     * @ghidraAddress 0x001940d8
     */
    virtual void OnUnknownSlot3();
};
