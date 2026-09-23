#pragma once

#include <vector>

/**
 * Running tally a game session accumulates.
 *
 * `9GameStats` in the RTTI descriptor at `0x0086f620`, with no base, so the compiler places the
 * vptr after the data at `+0x3c` and the class is 0x40 bytes. Its vtable is at `0x007cd738` and has
 * two entries, the compiler-generated type function and the destructor, so the destructor is the
 * only virtual the class declares.
 *
 * The size, the vptr offset, and the three vectors all fall out of the constructor at `0x0010f150`
 * and the destructor at `0x0010b648` together. The destructor releases the three vectors in reverse
 * declaration order and frees the object under the tag `GameStats`, which is what confirms the
 * class name against the descriptor independently.
 *
 * GameManagerImpl embeds one at `+0x28` and vends its address through vtable slot 19. The
 * GrooveWorld constructor receives that address and stores it, so the world writes the tally while
 * the manager owns it.
 *
 * Each vector has a four-byte element, which the destructor proves by dividing the byte span by
 * four to recover the count. The element type itself is not recovered, and `int` stands in for it.
 * Every member is private, and the purpose of each is unrecovered. GrooveWorld writes mUnknown14
 * directly, which a friend declaration models; a public member fits the image equally well.
 */
class GameStats {
    // GrooveWorld::MarkStatsFlag() at 0x00195378 writes mUnknown14 directly.
    friend class GrooveWorld;

public:
    /**
     * Start with every counter clear and all three vectors empty.
     *
     * The constructor does not write mUnknown08, mUnknown0c, or mUnknown14, so a tally starts with
     * three indeterminate fields.
     *
     * @ghidraAddress 0x0010f150
     */
    GameStats();

    /**
     * Release the three vectors.
     *
     * @ghidraAddress 0x0010b648
     */
    virtual ~GameStats();

private:
    int mUnknown00; // +0x00
    int mUnknown04; // +0x04
    // Not written by the constructor.
    int mUnknown08; // +0x08
    // Not written by the constructor.
    int mUnknown0c; // +0x0c
    int mUnknown10; // +0x10
    // Not written by the constructor.
    int mUnknown14;              // +0x14
    std::vector<int> mUnknown18; // +0x18
    std::vector<int> mUnknown24; // +0x24
    std::vector<int> mUnknown30; // +0x30
};
