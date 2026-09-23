#pragma once

#include <vector>

#include "os/hxstr.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

struct MetRemixRecord;

/**
 * One queued remix.
 *
 * load() allocates each entry as 0x0c bytes, reads the name into `+0x00`, and reads the flag into
 * `+0x08` through `operator>>(IBStream &, int &)`. MetRemixManager loads a remix whose flag is set
 * from the factory files and one whose flag is clear from the memory card, which is the evidence
 * for the flag's name. The struct and both member names are inferred.
 */
struct JukeboxPlayListEntry {
    HxStr name;  /*!< The remix name. +0x00 */
    int factory; /*!< Non-zero for a factory remix. +0x08 */
};

/**
 * Ordered list of the remixes queued in the jukebox.
 *
 * `15JukeboxPlayList` in the RTTI descriptor at `0x0086f7a0`, a leaf class with no base. The class
 * name is the RTTI spelling verbatim. Following the g++ 2.x layout for a class with no base, the
 * vptr sits after the data members at `+0x0c`, and the object is 0x10 bytes, so the vector below is
 * the whole of its state. The four-entry vtable at `0x007e6d90` runs the compiler-generated
 * GetTypeInfo at `0x001e5ca8`, the destructor, and the two stream members.
 *
 * MetRemixManager embeds one at `+0xc4`, and its constructor expands the inline constructor below.
 * MetJukeboxEditPlaylistScreen addresses that embedded instance through its own `+0xc4` and counts
 * its rows from it. Two memcard tasks persist it, LoadJukeboxPlayListMCT and
 * SaveJukeboxPlayListMCT.
 *
 * The list owns its entries. clear() releases each one with a delete expression before emptying
 * the vector, which is what establishes the element as owned rather than borrowed.
 *
 * No member name is attested anywhere in the image, so every identifier below follows the required
 * style. The member is public because MetJukeboxEditPlaylistScreen::GetItemCount() reads the
 * vector bounds directly and no accessor for it survives anywhere in the image. A friend
 * declaration fits that access equally well, and the ambiguity is recorded rather than resolved.
 */
class JukeboxPlayList {
public:
    /**
     * Construct an empty list.
     *
     * Inline. MetRemixManager's constructor expands it, writing the vptr and the empty vector and
     * then running clear(). The one out-of-line copy has no caller.
     *
     * @ghidraAddress 0x001e5ce8
     */
    JukeboxPlayList() {
        clear();
    }

    /**
     * Release every entry and the vector.
     *
     * @ghidraAddress 0x001e5da8
     */
    virtual ~JukeboxPlayList();

    /**
     * Write the list to a stream.
     *
     * Vtable slot 2. Writes the record version 1, then the entry count, then each entry as its
     * name's length, the name's bytes, and the factory flag. The name is inferred from the stream
     * slot it reaches, which MetPersonaData::Save() settles as OBStream slot 4.
     *
     * @param pStream The stream to write to.
     * @ghidraAddress 0x001e1e38
     */
    virtual void save(OBStream *pStream);

    /**
     * Read the list back from a stream.
     *
     * Vtable slot 3. The counterpart of save(). A version of zero or less leaves the list as it is.
     * Otherwise the vector is resized to the entry count, and each slot receives a fresh entry. The
     * entries the list held before are not released.
     *
     * @param pStream The stream to read from.
     * @ghidraAddress 0x001e1f70
     */
    virtual void load(IBStream *pStream);

    /**
     * Release every entry and empty the list.
     *
     * Not virtual, and it occupies no table slot. The destructor runs it first and the
     * MetRemixManager constructor runs it on its embedded instance. The name is inferred from the
     * two things the routine does.
     *
     * @ghidraAddress 0x001e2248
     */
    void clear();

    /**
     * Drop every entry the remix catalogue no longer knows.
     *
     * Each entry is looked up through MetRemixManager::FindRecord(), and an entry with no record is
     * erased from the vector without being released. The title is inferred.
     *
     * @ghidraAddress 0x001e5f10
     */
    void RemoveUnknownEntries();

    /**
     * Return one entry by position.
     *
     * The position is found by walking the vector from its start. The title is inferred.
     *
     * @param nIndex The position, counted from zero.
     * @return The entry, or null for an empty list or a position past the end.
     * @ghidraAddress 0x001e5ec0
     */
    JukeboxPlayListEntry *GetEntry(int nIndex);

    /**
     * Append an entry for one remix.
     *
     * The entry takes the record's `+0x08` string as its name and its `+0x24` word as its factory
     * flag. MetJukeboxBaseScreen's slot 19 is the caller. The title is inferred.
     *
     * @param record The remix to queue.
     * @ghidraAddress 0x001e21b8
     */
    void AddEntry(const MetRemixRecord &record);

    /**
     * Release and remove the entry at one position.
     *
     * A position past the end does nothing. MetJukeboxEditPlaylistScreen's slot 19 is the caller.
     * The title is inferred.
     *
     * @param nIndex The position, counted from zero.
     * @ghidraAddress 0x001e20f8
     */
    void RemoveEntry(int nIndex);

    /**
     * Exchange the entries at two positions.
     *
     * Neither position is checked. MetJukeboxEditPlaylistScreen's slot 19 moves an entry up and
     * down through it. The title is inferred.
     *
     * @param nFirst The first position.
     * @param nSecond The second position.
     * @ghidraAddress 0x001e5fa0
     */
    void SwapEntries(int nFirst, int nSecond);

    /** The queued remixes, owned by the list. +0x00 */
    std::vector<JukeboxPlayListEntry *> entries;
};
