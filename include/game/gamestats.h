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
 * Reset() sizes the three per-player vectors, and Gamer fills them when a game ends. The metagame
 * statistics screens read them back through the accessors, printing a score and a tally with `%d`
 * and a progress and a ratio, each scaled by 100, with `%3d%%`. GrooveWorld writes mUnknown14
 * directly, which a friend declaration models; a public member fits the image equally well.
 */
class GameStats {
    // GrooveWorld::MarkStatsFlag() at 0x00195378 writes mUnknown14 directly.
    friend class GrooveWorld;

public:
    /**
     * Start with every counter clear and all three vectors empty.
     *
     * The constructor does not write mUnknown08, mProgress, or mUnknown14, so a tally starts with
     * three indeterminate fields.
     *
     * @ghidraAddress 0x0010f150
     */
    GameStats();

    /**
     * Release the three vectors.
     *
     * The GameStats unit also emits an unreferenced, byte-identical copy at `0x0010fda0`.
     *
     * @ghidraAddress 0x0010b648
     */
    virtual ~GameStats();

    /**
     * Clear the tally for a new game and give each player a zero entry in every vector.
     *
     * The title is inferred.
     *
     * @param nPlayers The number of players.
     * @ghidraAddress 0x0010f1a8
     */
    void Reset(int nPlayers);

    /**
     * @param nPlayer The player's index.
     * @return The player's final score.
     * @ghidraAddress 0x0010ff20
     */
    int GetScore(int nPlayer);

    /**
     * @param nPlayer The player's index.
     * @param nScore The player's final score.
     * @ghidraAddress 0x0010ff38
     */
    void SetScore(int nPlayer, int nScore);

    /**
     * @return The fraction of the song reached, 1.0 when it was completed.
     * @ghidraAddress 0x0010ff50
     */
    float GetProgress();

    /**
     * @param flProgress The fraction of the song reached.
     * @ghidraAddress 0x0010ff58
     */
    void SetProgress(float flProgress);

    /**
     * @param nPlayer The player's index.
     * @return The player's Player::Slot18() fraction at the end of the game.
     * @ghidraAddress 0x0010ff60
     */
    float GetRatio(int nPlayer);

    /**
     * @param nPlayer The player's index.
     * @param flRatio The player's Player::Slot18() fraction.
     * @ghidraAddress 0x0010ff78
     */
    void SetRatio(int nPlayer, float flRatio);

    /**
     * @param nPlayer The player's index.
     * @return The player's Player::Slot17() count at the end of the game.
     * @ghidraAddress 0x0010ff90
     */
    int GetTally(int nPlayer);

    /**
     * @param nPlayer The player's index.
     * @param nTally The player's Player::Slot17() count.
     * @ghidraAddress 0x0010ffa8
     */
    void SetTally(int nPlayer, int nTally);

private:
    int mPlayerCount; // +0x00

public:
    /**
     * Non-zero when the solo song was completed.
     *
     * Public because Gamer writes it directly at `0x00111b9c`, and the image has no accessor.
     * +0x04
     */
    int mCompleted;

    /**
     * Copied from Gamer +0x98 when a solo game ends.
     *
     * Public because Gamer writes it directly at `0x00111ba8`, and the image has no accessor. Not
     * written by the constructor. +0x08
     */
    int mUnknown08;

private:
    // Not written by the constructor.
    float mProgress; // +0x0c
    int mUnknown10;  // +0x10

public:
    /**
     * Written by GrooveWorld. Not written by the constructor.
     *
     * Public because MetRenderer::OnFreqEnded() reads it directly at `0x0036c138`, where a
     * non-zero value sends a finished jam to the end screen rather than MetRemixTypeScreen, and the
     * image has no accessor. +0x14
     */
    int mUnknown14;

private:
    std::vector<int> mScores;   // +0x18
    std::vector<float> mRatios; // +0x24
    std::vector<int> mTallies;  // +0x30
};
