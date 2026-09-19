#pragma once

#include "os/hxstr.h"

// Non-polymorphic 0xb0-byte class that FreqAppearance owns one of. Its constructor is at
// 0x00249c40, it stores no vtable, and it has a std::list at its own +0xa0. Nothing in the image
// names it, so the title here is inferred from its one owner. Only the pointer is needed, so the
// class stays incomplete rather than being given an invented layout.
class FreqAppearanceDetail;

/**
 * Appearance of a player's avatar.
 *
 * `14FreqAppearance` in the RTTI descriptor at `0x0086f580`, with no base, so the compiler places
 * the vptr after the data at `+0x10` and the class is 0x14 bytes. Its vtable is at `0x007d98a0`.
 *
 * The layout comes from the copy constructor at `0x00174668`, which builds the members and then
 * assigns from the source through the assignment operator rather than copying them one by one.
 * That constructor is also what establishes the ownership: it takes 0xb0 bytes from the allocator
 * for the detail object and stores the pointer at `+0x08`, and the assignment operator releases
 * and replaces it. The purpose of each member is not recovered, and no reader has been traced, so
 * every member is private.
 */
class FreqAppearance {
public:
    /**
     * @param other The appearance to copy.
     * @ghidraAddress 0x00174668
     */
    FreqAppearance(const FreqAppearance &other);

    /**
     * Replace this appearance with a copy of another.
     *
     * Self-assignment is tested first and does nothing. Otherwise the name is assigned and the
     * detail object at `+0x08` is released and replaced.
     *
     * @param other The appearance to copy.
     * @return This appearance.
     * @ghidraAddress 0x001748e0
     */
    FreqAppearance &operator=(const FreqAppearance &other);

private:
    HxStr mUnknown00;              // +0x00 constructed from a literal at 0x007d97b8
    FreqAppearanceDetail *mDetail; // +0x08 owned, 0xb0 bytes
    int mUnknown0c;                // +0x0c cleared on construction
};
