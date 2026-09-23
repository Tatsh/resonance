#pragma once

#include <vector>

#include "os/hxstr.h"

namespace Rnd {
class Tex;
} // namespace Rnd

/**
 * Pair of textures a screen alternates between, one shown while the other loads.
 *
 * The class is not polymorphic and emits no RTTI, so no descriptor, no vtable, and no allocation
 * tag identifies it. Most instances are by-value members of a front-end screen, and
 * MetArenasScreen allocates its one from the untagged heap, so the tag lever does not apply. The
 * class name is inferred and agrees with the title the Ghidra program already records for the
 * constructor. No method name and no member name is attested anywhere in the image, so every
 * identifier below follows the required style rather than a recovered spelling.
 *
 * Six screens embed a pair of these records, and each pair is built from the texture names
 * `gSongLogo1.tex` with `gSongLogo2.tex` and `gSongLabel1.tex` with `gSongLabel2.tex`.
 * MetArenasScreen, MetJukeboxBaseScreen, MetMultiEndRemixScreen, MetRemixDataScreen,
 * MetSoloEndRemixScreen, and MetSoloStagesScreen are the six. Three of their headers record the
 * record as a reserved 0x30-byte span, which predates this declaration.
 *
 * The record is 0x30 bytes. ResolveTextures() fills the vector with the two textures, each through
 * Rnd::Manager::Find() narrowed to Rnd::Tex. Load() points the pending texture at a bitmap, and
 * Advance() swaps the pending texture into the current one once its load has finished.
 */
class TexturePairRecord {
public:
    /**
     * Record the two texture names and start with no resolved texture.
     *
     * Both names are copied into the record. The four integers below the object vector start at
     * zero except mPending, which starts at one.
     *
     * @param first The first texture name.
     * @param second The second texture name.
     * @ghidraAddress 0x00246de0
     */
    TexturePairRecord(const HxStr &first, const HxStr &second);

    /**
     * Release the two names and the texture vector.
     *
     * MetArenasScreen's destructor and MetJukeboxBaseScreen's run the deleting form.
     *
     * @ghidraAddress 0x001fc568
     */
    ~TexturePairRecord();

    /**
     * Mark the record as needing its current texture loaded again.
     *
     * Writes one to mInvalid and does nothing else. The method name is inferred from the one field
     * the routine writes and from its call sites, which run it whenever a screen becomes visible.
     *
     * @ghidraAddress 0x002498c0
     */
    void invalidate();

    /**
     * Resolve both textures by name, once.
     *
     * The title is inferred.
     *
     * @ghidraAddress 0x00246ef0
     */
    void ResolveTextures();

    /**
     * Point the pending texture at a bitmap and start its load.
     *
     * The bitmap is configured at 16 bits per pixel with the texture's own mip selector and flags,
     * and the load runs with no zone current. The title is inferred.
     *
     * @param path The bitmap path.
     * @ghidraAddress 0x00249760
     */
    void Load(const HxStr &path);

    /**
     * Report the current texture.
     *
     * The title is inferred.
     *
     * @return The current texture, or null while the record is invalid.
     * @ghidraAddress 0x00249808
     */
    Rnd::Tex *Current();

    /**
     * Make the pending texture current once its load has finished.
     *
     * The title is inferred.
     *
     * @return 1 when the textures were swapped, and 0 when no load was pending or it is still
     *         running.
     * @ghidraAddress 0x00249850
     */
    int Advance();

    /**
     * Build the path of a song's logo bitmap.
     *
     * The name `random` selects the shared random logo. The title is inferred.
     *
     * @param name The song's level name.
     * @return The path.
     * @ghidraAddress 0x00247038
     */
    static HxStr LogoPath(const HxStr &name);

    /**
     * Build the path of a song's picture bitmap.
     *
     * The name `random` selects the shared random picture. The title is inferred.
     *
     * @param name The song's level name.
     * @return The path.
     * @ghidraAddress 0x00247250
     */
    static HxStr PicturePath(const HxStr &name);

    /**
     * Build the path of an arena's screenshot bitmap.
     *
     * The name `random` selects the shared random picture. MetArenasScreen is the one caller. The
     * title is inferred.
     *
     * @param name The arena name.
     * @return The path.
     * @ghidraAddress 0x00247468
     */
    static HxStr ArenaPath(const HxStr &name);

private:
    std::vector<Rnd::Tex *> mTextures; // +0x00
    int mCurrent;                      // +0x0c, the index of the texture shown
    int mPending;                      // +0x10, the index of the texture loading, starts at 1
    int mLoading;                      // +0x14, set while a load is pending
    int mResolved;                     // +0x18, set once the textures are resolved
    HxStr mFirstName;                  // +0x1c
    HxStr mSecondName;                 // +0x24
    int mInvalid;                      // +0x2c
};
