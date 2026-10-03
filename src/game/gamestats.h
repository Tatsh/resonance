#pragma once

#include <vector>

/**
 * Running tally a game session accumulates.
 *
 * Its RTTI descriptor is at `0x0086f620`. It has no base. The compiler places the vptr after the
 * data at `+0x3c`, and the class is 0x40 bytes. Its vtable is at `0x007cd738` and has two entries,
 * the compiler-generated type function and the destructor. The destructor is the only virtual the
 * class declares.
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
 * and a progress and a ratio, each scaled by 100, with `%3d%%`. GrooveWorld writes mRemixEdited
 * directly, which a friend declaration models; a public member fits the image equally well.
 */
class GameStats {
    // GrooveWorld::MarkStatsFlag() at 0x00195378 writes mRemixEdited directly.
    friend class GrooveWorld;

public:
    /**
     * Start with every counter clear and all three vectors empty.
     *
     * The constructor does not write mCheated, mProgress, or mRemixEdited. A tally starts with
     * three indeterminate fields.
     *
     * @ghidraAddress NTSC-U/C: 0x0010f150
     * @ghidraAddress PAL: 0x0010f5b0
     */
    GameStats();

    /**
     * Release the three vectors.
     *
     * The GameStats unit also emits an unreferenced, byte-identical copy at `0x0010fda0`.
     *
     * @ghidraAddress NTSC-U/C: 0x0010b648
     * @ghidraAddress PAL: 0x0010b7d0
     */
    virtual ~GameStats();

    /**
     * Clear the tally for a new game and give each player a zero entry in every vector.
     *
     * The title is inferred.
     *
     * @param nPlayers The number of players.
     * @ghidraAddress NTSC-U/C: 0x0010f1a8
     * @ghidraAddress PAL: 0x0010f608
     */
    void Reset(int nPlayers);

    /**
     * @param nPlayer The player's index.
     * @return The player's final score.
     * @ghidraAddress NTSC-U/C: 0x0010ff20
     * @ghidraAddress PAL: 0x00110380
     */
    int GetScore(int nPlayer);

    /**
     * @param nPlayer The player's index.
     * @param nScore The player's final score.
     * @ghidraAddress NTSC-U/C: 0x0010ff38
     * @ghidraAddress PAL: 0x00110398
     */
    void SetScore(int nPlayer, int nScore);

    /**
     * @return The fraction of the song reached, 1.0 when it was completed.
     * @ghidraAddress NTSC-U/C: 0x0010ff50
     * @ghidraAddress PAL: 0x001103b0
     */
    float GetProgress();

    /**
     * @param flProgress The fraction of the song reached.
     * @ghidraAddress NTSC-U/C: 0x0010ff58
     * @ghidraAddress PAL: 0x001103b8
     */
    void SetProgress(float flProgress);

    /**
     * @param nPlayer The player's index.
     * @return The player's Player::GetCaptureRatio() fraction at the end of the game.
     * @ghidraAddress NTSC-U/C: 0x0010ff60
     * @ghidraAddress PAL: 0x001103c0
     */
    float GetRatio(int nPlayer);

    /**
     * @param nPlayer The player's index.
     * @param flRatio The player's Player::GetCaptureRatio() fraction.
     * @ghidraAddress NTSC-U/C: 0x0010ff78
     * @ghidraAddress PAL: 0x001103d8
     */
    void SetRatio(int nPlayer, float flRatio);

    /**
     * @param nPlayer The player's index.
     * @return The player's Player::GetBestStreak() count at the end of the game.
     * @ghidraAddress NTSC-U/C: 0x0010ff90
     * @ghidraAddress PAL: 0x001103f0
     */
    int GetTally(int nPlayer);

    /**
     * @param nPlayer The player's index.
     * @param nTally The player's Player::GetBestStreak() count.
     * @ghidraAddress NTSC-U/C: 0x0010ffa8
     * @ghidraAddress PAL: 0x00110408
     */
    void SetTally(int nPlayer, int nTally);

    /**
     * The number of players the session records.
     *
     * Public because MetMultiStatsScreen::EnterAndShow() reads it directly, and the image has no
     * accessor. +0x00
     */
    int mPlayerCount;

    /**
     * Non-zero when the solo song was completed.
     *
     * Public because Gamer writes it directly at `0x00111b9c`, and the image has no accessor.
     * +0x04
     */
    int mCompleted;

    /**
     * Non-zero when a cheat ran during the solo game, copied from Gamer::mCheated.
     *
     * MetRenderer shows the stage-finish screen only for a completed game with this clear. Public
     * because Gamer writes it directly at `0x00111ba8`, and the image has no accessor. Not written
     * by the constructor. +0x08
     */
    int mCheated;

private:
    // Not written by the constructor.
    float mProgress; // +0x0c
    // Zeroed by the constructor and Reset(), and not read by a recovered routine.
    int mUnreadCounter; // +0x10

public:
    /**
     * Non-zero once the jam's phrases or remix effects changed. Not written by the constructor.
     *
     * GrooveWorld::MarkStatsFlag() sets it whenever PhraseMgr adds, installs, or clears a phrase
     * and whenever JamEffectsMgr applies a statistics-counted effect. Public because
     * MetRenderer::OnFreqEnded() reads it directly at `0x0036c138`, where a non-zero value sends a
     * finished jam to the end screen rather than MetRemixTypeScreen, and the image has no accessor.
     * +0x14
     */
    int mRemixEdited;

private:
    std::vector<int> mScores;   // +0x18
    std::vector<float> mRatios; // +0x24
    std::vector<int> mTallies;  // +0x30
};
