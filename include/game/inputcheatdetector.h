#pragma once

#include <vector>

#include "game/rawcontroller.h"
#include "os/hxstr.h"

/**
 * Controller reader that matches button sequences against a table of cheats.
 *
 * `18InputCheatDetector` in the RTTI descriptor at `0x008f2a40`, with RawController as its one
 * public base at offset 0. The object is 8 bytes, which both allocations fix (GrooveWorld's
 * constructor at `0x0018c00c` and MetaGameWorld's at `0x003d313c`), so the inherited vptr at
 * `+0x00` is followed by one member of its own. Its vtable at `0x007e6778` has four entries and a
 * zero terminator at index 4: the type function at `0x001dea88`, the destructor at `0x001deb00`,
 * the RawController override at `0x001dc658`, and slot 3, filled with the pure-virtual stub at
 * `0x005381a8`. The class is abstract, and InputCheatDetectorGS and InputCheatDetectorMet complete
 * it.
 *
 * The destructor at `0x001deb00` stores RawController's table and releases the object, which is
 * the implicitly declared destructor, so this class owes no definition.
 *
 * Only the declarations are written. The bodies sit in the gameplay range.
 */
class InputCheatDetector : public RawController {
public:
    /**
     * One cheat, as InputCheatDetector::RegisterCheats() at `0x001dabc8` builds it.
     *
     * The record is 0x14 bytes, the stride the detector's slot 2 walks the table by. Slot 2
     * compares the pressed buttons against mButtons and, on a match, passes mName to script event
     * 0xce. `CheatSequence` is a placeholder for the name.
     */
    struct CheatSequence {
        HxStr mName;               /*!< The script name the match reports. */
        std::vector<int> mButtons; /*!< The button sequence that triggers the cheat. */
    };

    /**
     * Attach the detector to a table of cheats.
     *
     * The body builds the shared tables through RegisterCheats() on first use, flagged by the word
     * at `0x00691db0`, and then empties the four input histories at `0x00691db8`, each 0x18 bytes
     * with a vector at `+0x08`. MetaGameWorld passes g_metCheatSequences, and GrooveWorld passes
     * g_gameCheatSequences. The static initialiser at `0x001de3e8` zeroes both, and the push-back
     * routines at `0x001deb90` and `0x001deb30` append 0x14-byte records to them.
     *
     * @param pCheats The table to match against.
     * @ghidraAddress 0x001daaf8
     */
    explicit InputCheatDetector(std::vector<CheatSequence> *pCheats);

    /**
     * Match one controller reading against the table. Slot 2.
     *
     * @param nUnknown1 The first word of the reading.
     * @param nUnknown2 The second word of the reading.
     * @param nUnknown3 The third word of the reading.
     * @param flUnknown4 The float of the reading.
     * @ghidraAddress 0x001dc658
     */
    virtual void OnUnknownSlot2(int nUnknown1, int nUnknown2, int nUnknown3, float flUnknown4);

    /**
     * Unrecovered. Slot 3, pure in this class.
     *
     * Both overrides, InputCheatDetectorGS's at `0x001940d8` and InputCheatDetectorMet's at
     * `0x003d4788`, are empty. The verb, the parameter list, and the return type are unrecovered.
     */
    virtual void OnUnknownSlot3() = 0;

private:
    // The table the constructor stores. +0x04
    std::vector<CheatSequence> *mCheats;
};

/**
 * Cheats the front end detects.
 *
 * MetaGameWorld's constructor passes the address to the InputCheatDetectorMet it builds. The name
 * follows that one user. The definition belongs to the detector's translation unit and is not
 * written.
 *
 * @ghidraAddress 0x00691e18
 */
extern std::vector<InputCheatDetector::CheatSequence> g_metCheatSequences;

/**
 * Cheats a game session detects.
 *
 * GrooveWorld's constructor passes the address to the detector it builds. The name follows that
 * one user. The definition belongs to the detector's translation unit and is not written.
 *
 * @ghidraAddress 0x00691e28
 */
extern std::vector<InputCheatDetector::CheatSequence> g_gameCheatSequences;
