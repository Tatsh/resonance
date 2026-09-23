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
 * The four per-controller input histories, the registered flag, and the 0.75-second timeout are
 * file-local to the detector's translation unit.
 */
class InputCheatDetector : public RawController {
public:
    /**
     * One cheat, as InputCheatDetector::RegisterCheats() at `0x001dabc8` builds it.
     *
     * The record is 0x14 bytes, the stride the detector's slot 2 walks the table by. Slot 2
     * compares the pressed buttons against mButtons and, on a match, passes mName to script event
     * 0xce. `CheatSequence` is a placeholder for the name.
     *
     * The implicit default constructor at `0x001de858` and copy constructor at `0x001de880` are
     * emitted out of line. The translation unit's static initialiser and destructor at
     * `0x001de3e8`, which constructs and destroys the two cheat tables and the four input
     * histories, is compiler-generated as well.
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
     * Only a joystick press counts (type `'joy '`, a button number below 100, and a value of at
     * least 0.1). The press joins the slot's history, which restarts when more than 0.75 seconds
     * passed since the previous press. Every cheat whose sequence the history now includes clears
     * the history and runs script template 0xce with the cheat's name and the zero-based slot.
     *
     * @param nType The reading's device type, a four-character code.
     * @param nSlot The controller's slot, from 1.
     * @param nButton The button number.
     * @param flValue The reading's value.
     * @ghidraAddress 0x001dc658
     */
    virtual void OnUnknownSlot2(int nType, int nSlot, int nButton, float flValue);

    /**
     * Unrecovered. Slot 3, pure in this class.
     *
     * Both overrides, InputCheatDetectorGS's at `0x001940d8` and InputCheatDetectorMet's at
     * `0x003d4788`, are empty. The verb, the parameter list, and the return type are unrecovered.
     */
    virtual void OnUnknownSlot3() = 0;

private:
    // Fills g_metCheatSequences with three cheats and g_gameCheatSequences with eleven, reusing
    // one stack record, and sets the registered flag.
    // 0x001dabc8
    void RegisterCheats();

    // Appends a copy of the cheat to g_gameCheatSequences. RegisterCheats() passes its own
    // receiver, which the body does not read. The title is inferred.
    // 0x001deb30
    void AddGameCheat(const CheatSequence &cheat);

    // Appends a copy of the cheat to g_metCheatSequences, as AddGameCheat() does.
    // 0x001deb90
    void AddMetCheat(const CheatSequence &cheat);

    // The table the constructor stores. +0x04
    std::vector<CheatSequence> *mCheats;
};

/**
 * Cheats the front end detects.
 *
 * MetaGameWorld's constructor passes the address to the InputCheatDetectorMet it builds. The name
 * follows that one user.
 *
 * @ghidraAddress 0x00691e18
 */
extern std::vector<InputCheatDetector::CheatSequence> g_metCheatSequences;

/**
 * Cheats a game session detects.
 *
 * GrooveWorld's constructor passes the address to the detector it builds. The name follows that
 * one user.
 *
 * @ghidraAddress 0x00691e28
 */
extern std::vector<InputCheatDetector::CheatSequence> g_gameCheatSequences;
