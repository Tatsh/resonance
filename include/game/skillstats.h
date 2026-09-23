#pragma once

#include "stream/ibstream.h"
#include "stream/obstream.h"

/**
 * Persisted result for one skill setting of one level.
 *
 * `10SkillStats` in the RTTI descriptor at `0x007d3db8`, with no base, so the compiler places the
 * vptr after the data at `+0x08` and the class is 0x0c bytes. Its vtable is at `0x007d3d20` and
 * runs the type function, the destructor, Save(), and Load().
 *
 * LevelStats stores exactly three of these and Save() at `0x00142760` writes all three without a
 * loop, so the count is fixed in the source rather than computed.
 *
 * Save() emits the beaten flag as a single truth byte and the high score as a 16-bit halfword,
 * and the version 1 branch of Load() reads the high score as a full word, so the narrowing arrived
 * with record version 2.
 *
 * Both members are public, because CampaignStats reads and writes them directly and the image has
 * no accessor for either.
 */
class SkillStats {
public:
    /**
     * Construct a record with no defined contents.
     *
     * Defined in the header rather than compiled out of line. Every construction in the image
     * stores only the vtable pointer, which is what establishes both that the constructor exists
     * and that it initialises neither member. LevelStats::Load() relies on that: it builds three
     * of these on the stack and hands each straight to Load().
     */
    SkillStats() {
    }

    /**
     * @ghidraAddress 0x001452f8
     */
    virtual ~SkillStats();

    /**
     * Write this record.
     *
     * Vtable slot 2. The version byte 2 precedes the payload, and it is written whether or not
     * the reader will consult it, because Load() branches on the version CampaignStats::Load()
     * stored in g_nStatsRecordVersion rather than on this byte.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x00145338
     */
    virtual void Save(OBStream &stream);

    /**
     * Read this record.
     *
     * Vtable slot 3. The branch is on g_nStatsRecordVersion, which CampaignStats::Load() fills
     * before it arrives at any element. A version other than 1 or 2 reads nothing, and both
     * members retain their previous values.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x00142f88
     */
    virtual void Load(IBStream &stream);

    /**
     * Non-zero once the level is beaten at this difficulty. CampaignStats::RecordLevelBeaten()
     * sets it. +0x00
     */
    int mBeaten;
    /**
     * The best score at this difficulty. CampaignStats::RecordHighScore() raises it. +0x04
     */
    int mHighScore;
};

/**
 * Version of the statistics record currently being read.
 *
 * CampaignStats::Load() reads the file's version word straight into this global, and LevelStats
 * and SkillStats then branch on it rather than receiving it as an argument. Nothing else in the
 * image writes it.
 *
 * @ghidraAddress 0x00672c80
 */
extern int g_nStatsRecordVersion;
