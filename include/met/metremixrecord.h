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
 * The byte at `+0x20` is stored with `sb` and its one recovered value is 1, which is why it is a
 * `bool`. No member name is attested anywhere in the image, so every identifier below follows the
 * required style.
 */
struct MetRemixRecord {
    MetRemixRecord()
        : unknown00_(""), name(""), unknown10_(""), unknown18_(""), unknown20_(true), factory(0),
          unknown34_(0) {
    }

    /**
     * Build a record from its parts, taken by value.
     *
     * ListRemixesMCT::OnFileLoaded() at `0x0017ece0` is the only site, and no out-of-line copy
     * exists. factory starts at zero.
     *
     * @param unknown00 Copied into unknown00_.
     * @param nameIn Copied into name.
     * @param unknown10 Copied into unknown10_.
     * @param unknown18 Copied into unknown18_.
     * @param bUnknown20 Stored in unknown20_.
     * @param appearancesIn Copied into appearances.
     * @param nUnknown34 Stored in unknown34_.
     */
    MetRemixRecord(HxStr unknown00,
                   HxStr nameIn,
                   HxStr unknown10,
                   HxStr unknown18,
                   bool bUnknown20,
                   std::vector<FreqAppearance> appearancesIn,
                   int nUnknown34)
        : unknown00_(unknown00), name(nameIn), unknown10_(unknown10), unknown18_(unknown18),
          unknown20_(bUnknown20), factory(0), appearances(appearancesIn), unknown34_(nUnknown34) {
    }

    HxStr unknown00_; /*!< Starts as a copy of the empty string. +0x00 */
    /**
     * The remix name, which JukeboxPlayList::AddEntry() copies into a playlist entry and
     * MetSaveRemix::OnRemixesListed() compares against the name being saved. +0x08
     */
    HxStr name;
    HxStr unknown10_; /*!< Starts as a copy of the empty string. +0x10 */
    HxStr unknown18_; /*!< Starts as a copy of the empty string. +0x18 */
    bool unknown20_;  /*!< Starts true. +0x20 */
    /** Non-zero for a factory remix. JukeboxPlayList::AddEntry() copies it. +0x24 */
    int factory;
    /** Appearances of the players who recorded the remix. +0x28 */
    std::vector<FreqAppearance> appearances;
    int unknown34_; /*!< Starts at zero. +0x34 */
};
