#pragma once

#include <list>

#include "game/freqpart.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

namespace Rnd {
class View;
}

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
 * The routines below are the entry points FreqAppearance reaches it through, and mView is the one
 * field FreqAppearance reads directly. The part list is at `+0xa0`, and the rest of the layout is
 * not recovered.
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

    /**
     * Report the parts the avatar is built from.
     *
     * Defined in the header. The out-of-line copy is called by FreqAppearance::RenderBurnTextures()
     * and by the routine at `0x00262270`.
     *
     * @return The part list.
     * @ghidraAddress 0x0024f230
     */
    std::list<FreqPart *> &parts() {
        return mParts;
    }

    /**
     * Pack every part into consecutive 8-byte records.
     *
     * The part list is also counted a second time and that count is discarded.
     * FreqAppearance::Pack() is the one caller.
     *
     * @param pOut The first record to write.
     * @param pCount Receives the number of parts written.
     * @ghidraAddress 0x0024c398
     */
    void pack(FreqPart::Packed *pOut, int *pCount);

    /**
     * Append parts unpacked from consecutive 8-byte records.
     *
     * For each record the body allocates a FreqPart, unpacks it, clones a mesh for it through the
     * FreQ maker asset manager, hangs the mesh from mView, colours the part, and resets the
     * placement state of this object. FreqAppearance::Unpack() is the one caller. The body is not
     * written.
     *
     * @param pRecords The first record to read.
     * @param nCount The number of records.
     * @ghidraAddress 0x0024b898
     */
    void unpack(const FreqPart::Packed *pRecords, int nCount);

private:
    // Built by the routine at 0x0024f300 at the start of the constructor.
    unsigned char mUnknown00[0xa0]; // +0x00
    std::list<FreqPart *> mParts;   // +0xa0

public:
    /**
     * The avatar view the constructor allocates, 0x120 bytes. +0xa4
     *
     * Public because FreqAppearance::AttachToBurnSlot() reads it directly, and the image has no
     * accessor for it.
     */
    Rnd::View *mView;

private:
    unsigned char mUnknowna8[0x8]; // +0xa8
};
