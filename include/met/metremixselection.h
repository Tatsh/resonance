#pragma once

#include "os/hxstr.h"

/**
 * One remix a memory-card screen has picked out, or has yet to pick.
 *
 * The record is 0x18 bytes and is not polymorphic, so it emits no RTTI descriptor and no literal
 * in the image identifies it. The name here is inferred from its role.
 *
 * Three instances fix the layout, all initialised by the same five-store run. MetRemixDelScreen
 * builds one at `+0x108` and a second at `+0x120`, exactly 0x18 apart and from the same empty
 * literal at `0x00805ca8`, and its destructor releases the two name buffers in reverse order.
 * MetSaveRemix initialises the same five fields at `+0x94` from the empty literal at `0x00809840`
 * and releases the name in its own destructor. Two of the three fields below start at -1, which is
 * the no-selection sentinel the same screens use for a MetButtonList index.
 *
 * The layout was recovered twice independently, here from MetRemixDelScreen and by the jukebox
 * band from MetSaveRemix, and the two readings agree field for field on the initial values.
 * MetSaveRemix slot 39 takes the record as an argument, which is a use MetRemixDelScreen does not
 * show.
 *
 * Its constructor is inline, which is why it has no address of its own. Whether the original wrote
 * that constructor or assigned the five fields at each of the three sites is not settled by the
 * image, because the compiler emits the same stores either way and emits them out of offset order
 * at every site. The constructor form is reconstructed here, because the three sites produce
 * identical values.
 *
 * No member name is attested anywhere in the image, so every identifier below follows the required
 * style. The record has no behaviour beyond that initialisation, so it is a `struct` with public
 * members.
 */
struct MetRemixSelection {
    MetRemixSelection() : unknown00_(-1), name(""), unknown0c_(-1), unknown10_(-1), unknown14_(0) {
    }

    int unknown00_; /*!< Starts at -1. +0x00 */
    HxStr name;     /*!< Starts as a copy of the empty string. +0x04 */
    int unknown0c_; /*!< Starts at -1. +0x0c */
    int unknown10_; /*!< Starts at -1. +0x10 */
    int unknown14_; /*!< Starts at zero. +0x14 */
};
