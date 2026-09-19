#pragma once

#include "stream/ibstream.h"
#include "stream/obstream.h"

/**
 * Avatar an appearance stores apart from its username.
 *
 * The class is not polymorphic and emits no RTTI descriptor, so no accessor and no vtable names it.
 * Nor does the lever of last resort reach it. FreqAppearance's copy constructor takes its 0xb0
 * bytes from the untagged allocator at `0x004a81e0`, which passes no class name, and the two tagged
 * allocations inside its own constructor are billed to `stl_list` and to `Rnd::View`. The title
 * here is inferred from its one owner and from the diagnostic literal ` Freq=` that
 * FreqAppearance::Print writes where this object would appear.
 *
 * The object is 0xb0 bytes with a std::list at `+0xa0` and a Rnd::View of 0x120 bytes at `+0xa4`.
 * Only the six members below are recovered, and each is the entry point FreqAppearance reaches it
 * through. The layout is not declared, because nothing in FreqAppearance's own code touches a field
 * of this class directly.
 *
 * No identifier here is attested by the image, so every member takes the required style rather than
 * the CamelCase the classes around it use. That divergence is deliberate.
 */
class FreqAppearanceDetail {
public:
    /**
     * Start with an empty list and a fresh view.
     *
     * @ghidraAddress 0x00249c40
     */
    FreqAppearanceDetail();

    /**
     * Release the view and return the list node to the pool.
     *
     * The destructor is not virtual. FreqAppearance's own destructor releases this object through
     * a direct call rather than through a table.
     *
     * @ghidraAddress 0x0024edc8
     */
    ~FreqAppearanceDetail();

    /**
     * Write the avatar to a stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x0024b160
     */
    void save(OBStream &stream);

    /**
     * Read the avatar back from a stream.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x0024b340
     */
    void load(IBStream &stream);

    /**
     * Discard the contents, retaining the object.
     *
     * The destructor runs this before releasing the view, and FreqAppearance::operator=() runs it
     * before copying the source in. The name is inferred from those two uses.
     *
     * @ghidraAddress 0x0024a4b0
     */
    void clear();

    /**
     * Copy another avatar's contents in.
     *
     * The routine inserts into a std::map, which the `stl_maptree` allocation tag establishes. It
     * does not clear first, so FreqAppearance::operator=() pairs it with clear(). The name is
     * inferred from that pairing.
     *
     * @param other The avatar to copy.
     * @ghidraAddress 0x0024a088
     */
    void copyFrom(const FreqAppearanceDetail &other);
};
