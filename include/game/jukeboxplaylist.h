#pragma once

#include <vector>

#include "os/hxstr.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

/**
 * Ordered list of the remixes queued in the jukebox.
 *
 * `15JukeboxPlayList` in the RTTI descriptor at `0x0086f7a0`, a leaf class with no base. The class
 * name is the RTTI spelling verbatim. Following the g++ 2.x layout for a class with no base, the
 * vptr sits after the data members at `+0x0c`, and the object is 0x10 bytes, so the vector below is
 * the whole of its state. The four-entry vtable at `0x007e6d90` runs the compiler-generated
 * GetTypeInfo at `0x001e5ca8`, the destructor, and the two stream members.
 *
 * MetRemixManager embeds one at `+0xc4` and runs clear() on it from its own constructor.
 * MetJukeboxEditPlaylistScreen addresses that embedded instance through its own `+0xc4` and counts
 * its rows from it. Two memcard tasks persist it, LoadJukeboxPlayListMCT and
 * SaveJukeboxPlayListMCT.
 *
 * The list owns its entries. clear() releases each one with a delete expression before emptying
 * the vector, which is what establishes the element as an owned `HxStr *` rather than a borrowed
 * pointer. save() writes a record version of 1, then the entry count, then each entry as its
 * length followed by its buffer, so an entry is a remix name rather than an index.
 *
 * No member name is attested anywhere in the image, so every identifier below follows the required
 * style. The member is public because MetJukeboxEditPlaylistScreen::GetItemCount() reads the
 * vector bounds directly and no accessor for it survives anywhere in the image. A friend
 * declaration fits that access equally well, and the ambiguity is recorded rather than resolved.
 */
class JukeboxPlayList {
public:
    /**
     * Release every entry and the vector.
     *
     * @ghidraAddress 0x001e5da8
     */
    virtual ~JukeboxPlayList();

    /**
     * Write the list to a stream.
     *
     * Vtable slot 2. Writes the record version 1, then the entry count, then each entry. The name
     * is inferred from the stream slot it reaches, which MetPersonaData::Save() settles as
     * OBStream slot 4.
     *
     * The body is not written. The per-entry part runs through two further stream slots whose
     * arguments are not recovered.
     *
     * @param pStream The stream to write to.
     * @ghidraAddress 0x001e1e38
     */
    virtual void save(OBStream *pStream);

    /**
     * Read the list back from a stream.
     *
     * Vtable slot 3. The counterpart of save(). The body is not written, for the same reason.
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

    /** The queued remix names, owned by the list. +0x00 */
    std::vector<HxStr *> entries;
};
