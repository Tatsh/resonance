#pragma once

#include <vector>

#include "game/freqappearance.h"
#include "os/hxstr.h"

/**
 * One remix in the catalogue, with the appearances of the players who recorded it.
 *
 * The record is 0x38 bytes and is not polymorphic, so it emits no RTTI descriptor and no vtable.
 * Every instance is a by-value member or a vector element, so the allocation tag lever declines as
 * well: the one allocation that reaches it is the vector buffer, tagged `stl_vector` at
 * `0x007ccfe8`, which is the container rather than the element. The name here is inferred from its
 * users, and every one of them is remix-related. MetRemixManager stores one at `+0x114`,
 * MetSaveRemix a vector of them at `+0xcc`, and MetJukeboxCustomRemixesScreen and
 * MetJukeboxFactoryRemixesScreen each divide that vector's byte span by 0x38 to obtain their row
 * count.
 *
 * The layout comes from two sources that agree. The inline constructor in MetRemixManager builds
 * the four strings from the empty literal at `0x008077d8`, writes one byte of 1 at `+0x20`, zeroes
 * `+0x24`, default-constructs the vector at `+0x28`, and zeroes `+0x34`. The destructor at
 * `0x00183fd0` walks the vector at `+0x28` in 0x14-byte steps running the FreqAppearance
 * destructor on each, deallocates the buffer, and then frees the four string buffers at `+0x1c`,
 * `+0x14`, `+0x0c`, and `+0x04`, which is reverse declaration order.
 *
 * That destructor is the compiler-generated one, because it releases exactly the five members that
 * need releasing and nothing else, so it is recorded here rather than declared. The constructor is
 * not compiler-generated, because an implicit one would not write 1 into the byte at `+0x20`.
 *
 * The byte at `+0x20` is a `char`, not a `bool`. ListRemixesMCT::OnFileLoaded() loads the GameOK
 * byte with `lb` at `0x0017ef30` and stores it unchanged with `sb` at `0x0017ef78`, with no
 * normalisation to 0 or 1 in between. The member names follow the RemixIndex element fields that
 * ListRemixesMCT::OnFileLoaded() copies into them.
 */
struct MetRemixRecord {
    MetRemixRecord()
        : levelName(""), name(""), fileName(""), dateTime(""), gameOk(1), factory(0),
          albumNumber(0) {
    }

    /**
     * Build a record from its parts, taken by value.
     *
     * ListRemixesMCT::OnFileLoaded() at `0x0017ece0` is the only site, and no out-of-line copy
     * exists. factory starts at zero.
     *
     * @param levelNameIn Copied into levelName.
     * @param nameIn Copied into name.
     * @param fileNameIn Copied into fileName.
     * @param dateTimeIn Copied into dateTime.
     * @param cGameOk Stored in gameOk.
     * @param appearancesIn Copied into appearances.
     * @param nAlbumNumber Stored in albumNumber.
     */
    MetRemixRecord(HxStr levelNameIn,
                   HxStr nameIn,
                   HxStr fileNameIn,
                   HxStr dateTimeIn,
                   char cGameOk,
                   std::vector<FreqAppearance> appearancesIn,
                   int nAlbumNumber)
        : levelName(levelNameIn), name(nameIn), fileName(fileNameIn), dateTime(dateTimeIn),
          gameOk(cGameOk), factory(0), appearances(appearancesIn), albumNumber(nAlbumNumber) {
    }

    /**
     * The level the remix was recorded on. It keys the song's logo, picture, and configuration
     * texts and becomes the level name when the remix plays. +0x00
     */
    HxStr levelName;
    /**
     * The remix name, which JukeboxPlayList::AddEntry() copies into a playlist entry and
     * MetSaveRemix::OnRemixesListed() compares against the name being saved. +0x08
     */
    HxStr name;
    /** The memory card file MetRemixManager loads the remix from. +0x10 */
    HxStr fileName;
    /** The date and time the remix was saved, shown in the jukebox date text. +0x18 */
    HxStr dateTime;
    /** The GameOK byte of the remix index entry. Starts as 1. +0x20 */
    char gameOk;
    /** Non-zero for a factory remix. JukeboxPlayList::AddEntry() copies it. +0x24 */
    int factory;
    /** Appearances of the players who recorded the remix. +0x28 */
    std::vector<FreqAppearance> appearances;
    /** The album the remix belongs to, compared against GetAlbumJukeboxValue(). +0x34 */
    int albumNumber;
};
