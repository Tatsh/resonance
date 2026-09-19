#pragma once

#include <vector>

#include "game/skillstats.h"
#include "os/hxstr.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

/**
 * Persisted result for one level, across every skill setting.
 *
 * `10LevelStats` in the RTTI descriptor at `0x007d3da8`, with no base, so the compiler places the
 * vptr after the data at `+0x18` and the class is 0x1c bytes. Its vtable is at `0x007d3d48` and
 * runs the type function, the destructor, Save(), and Load().
 *
 * CampaignStats stores a vector of these and resolves one by name, so mName is the record's key.
 *
 * Save() writes exactly three skill records and the version 2 branch of Load() reads exactly
 * three, each built on the stack and appended, so the three are written out in the source rather
 * than looped over.
 */
class LevelStats {
public:
    /**
     * Construct an empty record.
     *
     * The out-of-line body zeroes the name and the vector and stores the vtable pointer, all of
     * which the compiler generates, so the written body is empty. mUnknown08 is not among the
     * members it initialises.
     *
     * @ghidraAddress 0x001448b8
     */
    LevelStats();

    /**
     * @ghidraAddress 0x001424e8
     */
    virtual ~LevelStats();

    /**
     * Write this record.
     *
     * Vtable slot 2. The version byte 2 precedes the payload. The name goes out as its length
     * followed by its bytes, and an empty name hands g_szEmptyString to the stream in place of a
     * null buffer.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x00142760
     */
    virtual void Save(OBStream &stream);

    /**
     * Read this record.
     *
     * Vtable slot 3. The branch is on g_nStatsRecordVersion. Version 1 reads a leading word that
     * it discards, then the name, then mUnknown08 as a full word, then a skill count it resizes
     * the vector to. Version 2 reads a version byte, the name, mUnknown08 as a single byte, and
     * then exactly three skill records into a cleared vector. A version other than 1 or 2 reads
     * nothing.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x00142880
     */
    virtual void Load(IBStream &stream);

    /**
     * Identity of the level this record belongs to.
     *
     * CampaignStats::FindLevelIndex() at `0x00144ba8` compares this member directly with
     * HxStr::operator==() and no accessor stands between the two, which is what makes the member
     * public rather than private. A friend declaration on CampaignStats fits the image equally
     * well.
     */
    HxStr mName; /*!< The level's name. +0x00 */

private:
    // Written by Save() as a single byte and read back as a byte under record version 2 and as a
    // word under version 1. The constructor does not initialise it and its purpose is not
    // recovered.
    int mUnknown08;
    // Exactly three entries once a record has been read.
    std::vector<SkillStats> mSkills; // +0x0c
};
