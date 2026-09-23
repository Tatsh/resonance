#pragma once

#include <vector>

#include "game/levelstats.h"
#include "os/hxstr.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

/**
 * Persisted progress across the whole campaign.
 *
 * `13CampaignStats` in the RTTI descriptor at `0x007d3d98`, with no base, so the compiler places
 * the vptr after the data at `+0x13c` and the class is 0x140 bytes. Its vtable is at `0x007d3d70`
 * and runs the type function, the destructor, Save(), and Load(). MetPersonaData embeds one
 * instance at offset 0, which is what fixes the size from the outside as well.
 *
 * Sixty per-stage counters span `+0x10` to `+0xff`, each row six stages wide, and ResetCounters()
 * zeroes them in one loop. They are the level count of each stage, then three-by-six blocks, one
 * row per difficulty, of the completed level count, the stage-score flag, and the stage score. The
 * stride of 0x18 at which IsStageComplete(), GetStageScoreBeaten(), and GetStageScore() index them
 * fixes the blocks.
 *
 * Five vectors of level indices follow at `+0x100`, one per stage. RecountStageCompleted() and
 * RecountStageScore() walk the vector of the stage they recount.
 *
 * Several members are understood but not yet written here. RebuildLevelList() at `0x00140908`
 * clears the level vector and rebuilds it from the global level list, MergeLevelList() at
 * `0x00142070` runs the same pass after a load, and PrintLevels() at `0x001451e0` streams the
 * vector under the label `levels[`. Further members at `0x00140b40`, `0x00140ef8`, `0x00141578`,
 * `0x00142288`, `0x00142610`, `0x00142d70`, `0x00144c38`, and `0x00145068` belong to this class or
 * to LevelStats and are not yet apportioned between the two.
 */
class CampaignStats {
public:
    /** The number of difficulties, easy, normal, and expert. */
    static constexpr int kDifficultyCount = 3;

    /** The number of stages, the secret stage 6 included. */
    static constexpr int kStageCount = 6;

    /** The number of stages whose level indices the record stores, stages 1 through 5. */
    static constexpr int kIndexedStageCount = 5;

    /**
     * Construct empty progress.
     *
     * The vectors are zeroed by the compiler-generated member initialisation, and the written
     * body is the ResetCounters() call. The member at `+0x0c` is not initialised.
     *
     * @ghidraAddress 0x00140580
     */
    CampaignStats();

    /**
     * @ghidraAddress 0x00140768
     */
    virtual ~CampaignStats();

    /**
     * Write the whole campaign record.
     *
     * Vtable slot 2. The record version 2 precedes the level count, and each level writes itself.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x00145110
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the whole campaign record.
     *
     * Vtable slot 3. The version word goes straight into g_nStatsRecordVersion, which is how
     * LevelStats and SkillStats learn which layout to expect. The level vector is resized to the
     * stored count before any element reads itself.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x00141ed8
     */
    virtual void Load(IBStream &stream);

    /**
     * Find the record for a level by name.
     *
     * @param name The level's name.
     * @return The index, or the element count when no record matches.
     * @ghidraAddress 0x00144ba8
     */
    int FindLevelIndex(const HxStr &name);

    /**
     * Report whether a level has been beaten at a difficulty.
     *
     * The level is found by name, and an unknown name reads one record past the end.
     *
     * @param nDifficulty The difficulty, 0 through 2.
     * @param name The level's name.
     * @return SkillStats::mBeaten of the level at that difficulty.
     * @ghidraAddress 0x00144f08
     */
    int GetLevelBeaten(int nDifficulty, const HxStr &name);

    /**
     * Report a level's high score at a difficulty.
     *
     * The level is found as GetLevelBeaten() finds it.
     *
     * @param nDifficulty The difficulty, 0 through 2.
     * @param name The level's name.
     * @return SkillStats::mHighScore of the level at that difficulty.
     * @ghidraAddress 0x00144ca8
     */
    int GetLevelHighScore(int nDifficulty, const HxStr &name);

    /**
     * Record a level as beaten at a difficulty and recount what depends on it.
     *
     * A level already beaten changes nothing. Otherwise the level's stage (from
     * GetAlbumLevelStage()) is recounted for completion and for score, and the unlock level is
     * recomputed. MetStageFinishScreen's slot 5 is the caller.
     *
     * @param nDifficulty The difficulty, 0 through 2.
     * @param name The level's name.
     * @ghidraAddress 0x00141798
     */
    void RecordLevelBeaten(int nDifficulty, const HxStr &name);

    /**
     * Record a score for a level at a difficulty when it beats the high score.
     *
     * A new high score also recounts the stage score of the level's stage.
     *
     * @param nDifficulty The difficulty, 0 through 2.
     * @param name The level's name.
     * @param nScore The score.
     * @ghidraAddress 0x00144d68
     */
    void RecordHighScore(int nDifficulty, const HxStr &name, int nScore);

    /**
     * Report whether a stage's total score has reached its target at a difficulty.
     *
     * @param nDifficulty The difficulty, 0 through 2.
     * @param nStage The stage, counted from 1.
     * @return The flag RecountStageScore() last stored.
     * @ghidraAddress 0x00145028
     */
    int GetStageScoreBeaten(int nDifficulty, int nStage);

    /**
     * Report a stage's total score at a difficulty.
     *
     * @param nDifficulty The difficulty, 0 through 2.
     * @param nStage The stage, counted from 1.
     * @return The total RecountStageScore() last stored.
     * @ghidraAddress 0x00145048
     */
    int GetStageScore(int nDifficulty, int nStage);

    /**
     * Report whether every required level of a stage is beaten at a difficulty.
     *
     * Easy has stages 1 through 3, normal stages 1 through 4, and expert stages 1 through 5, and a
     * stage outside that range is never complete. A stage whose album value
     * (GetAlbumLevelValue()) is set requires one level fewer than it has, because its last level is
     * a bonus level.
     *
     * @param nDifficulty The difficulty, 0 through 2.
     * @param nStage The stage, counted from 1.
     * @return 1 when the stage is complete, otherwise 0.
     * @ghidraAddress 0x00144e58
     */
    int IsStageComplete(int nDifficulty, int nStage);

    /**
     * Report whether every stage of a difficulty is complete and every bonus level beaten.
     *
     * @param nDifficulty The difficulty, 0 through 2.
     * @return 1 when the difficulty is complete, otherwise 0.
     * @ghidraAddress 0x00140d40
     */
    int IsDifficultyComplete(int nDifficulty);

    /**
     * Report the name of a stage's bonus level.
     *
     * The bonus level is the last level of the stage's list. The body does not read this object.
     *
     * @param nDifficulty The difficulty, 0 through 2.
     * @param nStage The stage, counted from 1.
     * @return The name, or an empty string when the stage is empty or has no bonus level at that
     * difficulty.
     * @ghidraAddress 0x00141690
     */
    HxStr GetBonusLevelName(int nDifficulty, int nStage);

    /**
     * Report whether the secret stage is unlocked.
     *
     * Expert must be complete and stage 6 must have levels.
     *
     * @return 1 when unlocked, otherwise 0.
     * @ghidraAddress 0x00144fc8
     */
    int IsSecretUnlocked();

    /**
     * Report whether the super secret is unlocked.
     *
     * The secret stage must be unlocked, the first level of stage 6 beaten on expert, and stage 6
     * must have at least two levels.
     *
     * @return 1 when unlocked, otherwise 0.
     * @ghidraAddress 0x001410f0
     */
    int IsSuperSecretUnlocked();

private:
    // 0x00141b90. Recount the beaten levels of one stage at one difficulty into
    // mStageCompleted. Each level's name is copied and discarded, which matches the binary.
    void RecountStageCompleted(int nDifficulty, int nStage);

    // 0x00141898. Recount the total high score of the beaten levels of one stage at one
    // difficulty into mStageScores, and set mStageScoreBeaten when the stage is complete and the
    // total reaches the stage's album value.
    void RecountStageScore(int nDifficulty, int nStage);

    // 0x00141cb8. Recompute mUnlockLevel from stage completion and return it.
    int UpdateUnlockLevel();

    // Zeroes the ten counter arrays, then counts the levels of the global level list into the
    // first of them. The body at 0x00140a68 consults the global level list and a further member
    // at 0x00145068, neither of which is identified, so it is declared here and not yet written.
    void ResetCounters();

    // Rebuilds the level vector from the global level list after a load. The body at 0x00142070
    // shares that pass with RebuildLevelList() and is not yet written.
    void MergeLevelList();

    std::vector<LevelStats> mLevels;
    // The progress UpdateUnlockLevel() last computed, 1 through 8. Not initialised by the
    // constructor.
    int mUnlockLevel;
    int mStageLevelCounts[kStageCount];
    int mStageCompleted[kDifficultyCount][kStageCount];
    int mStageScoreBeaten[kDifficultyCount][kStageCount];
    int mStageScores[kDifficultyCount][kStageCount];
    // The indices into mLevels of each stage's levels, for stages 1 through 5.
    std::vector<int> mStageLevels[kIndexedStageCount];
};
