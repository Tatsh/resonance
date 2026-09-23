#pragma once

#include <iostream>
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
     * which the compiler generates, so the written body is empty. mStage is not among the
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
     * it discards, then the name, then mStage as a full word, then a skill count it resizes
     * the vector to. Version 2 reads a version byte, the name, mStage as a single byte, and
     * then exactly three skill records into a cleared vector. A version other than 1 or 2 reads
     * nothing.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x00142880
     */
    virtual void Load(IBStream &stream);

    /**
     * Empty the name, clear mStage, and replace the skills with three cleared records.
     *
     * CampaignStats::RebuildLevelList() and MergeLevelList() run it on the record they append.
     * The title is inferred.
     *
     * @ghidraAddress 0x00142610
     */
    void Reset();

    /**
     * Copy another record's name, stage, and skills into this one.
     *
     * The skill vector is emptied, resized to the other's length with default records, and then
     * assigned element by element. CampaignStats::Assign() is the one caller. The routine writes no
     * return value, so it is not an assignment operator. The title is inferred.
     *
     * @param other The record to copy.
     * @ghidraAddress 0x00142d70
     */
    void Assign(const LevelStats &other);

    /**
     * Write this record to a text stream.
     *
     * The body is empty. CampaignStats::PrintLevels() calls it for each level. The title is
     * inferred.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x001452f0
     */
    void Print(std::ostream &stream) const;

    /**
     * Identity of the level this record belongs to.
     *
     * CampaignStats::FindLevelIndex() at `0x00144ba8` compares this member directly with
     * HxStr::operator==() and no accessor stands between the two, which is what makes the member
     * public rather than private. A friend declaration on CampaignStats fits the image equally
     * well.
     */
    HxStr mName; /*!< The level's name. +0x00 */

    /**
     * The album stage the level belongs to, counted from 1. +0x08
     *
     * CampaignStats::RebuildStageLevels() subtracts 1 and files the level's index under that
     * stage, and RebuildLevelList() and MergeLevelList() store GetAlbumLevelStage() of the name
     * here. Save() writes it as a single byte, and Load() reads it back as a byte under record
     * version 2 and as a word under version 1. The constructor does not initialise it. Public
     * because CampaignStats reads and writes it directly and the image has no accessor.
     */
    int mStage;

    /**
     * One result per difficulty, exactly three once a record has been read. +0x0c
     *
     * Public because CampaignStats reads and writes the entries directly from its level lookups
     * and recounts, and the image has no accessor for the vector.
     */
    std::vector<SkillStats> mSkills;
};
