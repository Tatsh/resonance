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
 * Ten six-entry counter arrays span `+0x10` to `+0xff`. The first four stand alone and the last
 * two are three-by-six blocks, which the two accessors at `0x00145028` and `0x00145048` establish
 * by indexing at a stride of 0x18 from `+0x70` and from `+0xb8`. Six is the width of every one of
 * them, and ResetCounters() zeroes all ten in one loop. None of the ten has a recovered purpose.
 *
 * Five vectors of int follow at `+0x100`, `+0x10c`, `+0x118`, `+0x124`, and `+0x130`. The
 * destructor releases all five and the constructor zeroes all five, and no other reader is
 * recovered, so they are recorded as a reserved span rather than declared individually.
 *
 * Several members are understood but not yet written here. RebuildLevelList() at `0x00140908`
 * clears the level vector and rebuilds it from the global level list, MergeLevelList() at
 * `0x00142070` runs the same pass after a load, PrintLevels() at `0x001451e0` streams the vector
 * under the label `levels[`, and the two indexed counter accessors sit at `0x00145028` and
 * `0x00145048`. Further members at `0x00140b40`, `0x00140d40`, `0x00140ef8`, `0x00141578`,
 * `0x00141690`, `0x00141798`, `0x00141898`, `0x00141b90`, `0x00141cb8`, `0x00142288`,
 * `0x00142610`, `0x00142d70`, `0x00144c38`, `0x00144ca8`, `0x00144d68`, `0x00144e58`,
 * `0x00144f08`, `0x00144fc8`, and `0x00145068` belong to this class or to LevelStats and are not
 * yet apportioned between the two.
 */
class CampaignStats {
public:
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

private:
    // Zeroes the ten counter arrays, then counts the levels of the global level list into the
    // first of them. The body at 0x00140a68 consults the global level list and a further member
    // at 0x00145068, neither of which is identified, so it is declared here and not yet written.
    void ResetCounters();

    // Rebuilds the level vector from the global level list after a load. The body at 0x00142070
    // shares that pass with RebuildLevelList() and is not yet written.
    void MergeLevelList();

    std::vector<LevelStats> mLevels; // +0x00
    int mUnknown0c;                  // +0x0c, not initialised by the constructor
    int mUnknown10[6];               // +0x10, counts of levels per skill setting
    int mUnknown28[6];               // +0x28
    int mUnknown40[6];               // +0x40
    int mUnknown58[6];               // +0x58
    int mUnknown70[3][6];            // +0x70
    int mUnknownb8[3][6];            // +0xb8
    // Five vectors of int, at +0x100, +0x10c, +0x118, +0x124, and +0x130. The constructor zeroes
    // all five and the destructor releases all five, and no further reader is recovered.
    unsigned char mUnknown100[60];
};
