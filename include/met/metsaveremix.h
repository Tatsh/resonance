#pragma once

#include <vector>

#include "memcard/memcarduser.h"
#include "met/metkbuser.h"
#include "met/metscreen.h"
#include "rnd/object.h"

/**
 * Base of the two screens that write a remix to a memory card.
 *
 * `13MetSaveRemix` in the RTTI descriptor at `0x008ef660`, with three public non-virtual bases at
 * fixed offsets, MetScreen at `+0x00`, MemcardUser at `+140`, and MetKBUser at `+144`. The object
 * is 0xe8 bytes, which both children fix independently. MetSaveRemixScreen starts its own members
 * at `+0xe8` and MetRemixDelScreen places its ListDataProvider base at `+232`.
 *
 * Two classes derive from the class, MetRemixDelScreen and MetSaveRemixScreen.
 *
 * Three vtables belong to the class. The primary table at `0x00809c00` has 43 entries, four more
 * than the MetScreen table, so the class declares four virtuals of its own at slots 39 through 42
 * at `0x00372488`, `0x0037a618`, `0x0037a620`, and `0x0037a628`. The last three sit eight bytes
 * apart and are two-instruction `jr ra` stubs, so three of the four are empty. The 21-entry
 * MemcardUser table at `0x00809b50` adjusts `this` by `-140` and the three-entry MetKBUser table
 * at `0x00809b30` adjusts it by `-144`.
 *
 * The constructor at `0x00372120` takes the renderer, the load priority, and the three names, and
 * forwards all five to MetScreen. It writes the three vptrs, zeroes the vector at `+0xac`, zeroes
 * the run of words from `+0xb8` through `+0xcc`, zeroes a second vector, and zeroes `+0xd8`,
 * `+0xdc`, and `+0xe4`. The word at `+0x94` and a nested object it addresses through `+0x0c`
 * through `+0x14` receive a value the digest does not resolve to a literal, so both are recorded
 * as undetermined.
 *
 * The destructor at `0x00372248` restores the two secondary vptrs, runs a routine at
 * `0x00183fd0`, restores the MetKBUser vptr to `0x007f16a0` and the MemcardUser vptr to
 * `0x007daf78`, runs the MetScreen destructor, and releases the object with the tag `MsgSink`.
 *
 * One inherited slot differs from the MetScreen table, slot 15 at `0x00375590`, which replaces an
 * empty MetScreen slot with the HxStr-taking handler that slot documents.
 */
class MetSaveRemix : public MetScreen, public MemcardUser, public MetKBUser {
public:
    /**
     * Construct the saver.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @param name The screen name.
     * @param directory The directory the container loads from.
     * @param file The container name, without the `.rnd` suffix.
     * @ghidraAddress 0x00372120
     */
    MetSaveRemix(MetRenderer *pRenderer,
                 int nPriority,
                 const HxStr &name,
                 const HxStr &directory,
                 const HxStr &file);

    /**
     * @ghidraAddress 0x00372248
     */
    virtual ~MetSaveRemix();

private:
    // Span from the end of the MetKBUser subobject to the vector at +0xac. The constructor writes
    // +0x94 and addresses a nested object through it, and neither is recovered.
    unsigned char mUnknown94[0x18];        // +0x94
    std::vector<Rnd::Object *> mUnknownac; // +0xac
    int mUnknownb8;                        // +0xb8
    int mUnknownbc;                        // +0xbc
    int mUnknownc0;                        // +0xc0
    int mUnknownc4;                        // +0xc4
    int mUnknownc8;                        // +0xc8
    std::vector<Rnd::Object *> mUnknowncc; // +0xcc
    int mUnknownd8;                        // +0xd8
    int mUnknowndc;                        // +0xdc
    int mUnknowne0;                        // +0xe0, not written by the constructor
    int mUnknowne4;                        // +0xe4
};
