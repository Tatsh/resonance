#pragma once

#include "memcard/memcarduser.h"
#include "met/metscreen.h"
#include "os/asynccallback.h"

/**
 * Manager of the remix catalogue, which also presents itself as a dialogue screen.
 *
 * `15MetRemixManager` in the RTTI descriptor at `0x008efc50`, with three public non-virtual bases
 * at fixed offsets, MetScreen at `+0x00`, MemcardUser at `+140`, and AsyncCallback at `+144`. Its
 * own file is `MetRemixManager.cpp`, which its asserts record at `0x00807bb0`, and it is one of
 * only two classes in the subsystem whose file name survives in the image.
 *
 * The rest of the game resolves the one instance through the accessor at `0x00361000`, which
 * forwards to `0x003610a8`, which runs the lazy initialiser at `0x00361210` and then returns the
 * cached pointer at `0x006c1110`. The initialiser resolves the instance by handing the registry
 * key `MetRemixManager`, at `0x00807a78`, to MetScreen::FindScreenByName(), so the manager is a
 * registered screen rather than a separately constructed singleton. The accessor is not declared
 * below, because the split across three routines does not resolve into one static member without
 * guessing which of the three the programmer wrote.
 *
 * Three vtables belong to the class, the 39-entry primary at `0x00807de0`, the 21-entry
 * MemcardUser table at `0x00807d30` that adjusts `this` by `-140`, and the three-entry
 * AsyncCallback table at `0x00807d10` that adjusts it by `-144`. The primary is the same length as
 * the MetScreen table, so the class declares no virtual of its own.
 *
 * Four entries of the primary table differ from the MetScreen table, and only the destructor has a
 * recovered name. They are 0 `0x00360788`, the compiler-generated GetTypeInfo, 1 `0x00355070`,
 * 5 `0x00361518`, 15 `0x003555c0`, and 36 `0x00361550`. An earlier reading counted five while
 * listing three. The MemcardUser table overrides its slots 5, 10, 11, 12, and 15 at `0x003569d0`,
 * `0x003573e8`, `0x003553e8`, `0x00355ff0`, and `0x003563e0`, and the AsyncCallback table
 * overrides its slot 2 at `0x00358310`.
 *
 * This declaration is deliberately partial and declares no data member. The constructor at
 * `0x00352b80` runs for roughly 300 instructions, and everything after the three vptr writes is
 * member initialisation. The recovered map follows. A red-black tree header occupies `+0x94`
 * through `+0x9c`, built by taking a 0x20-byte node from the STL pool and self-linking it, and a
 * second occupies `+0xa0` through `+0xa8` over a 0x18-byte node. A 0x20-byte node is a 0x10-byte
 * tree-node base plus a 0x10-byte value, and a 0x18-byte node is the same base plus an 8-byte
 * value, which is the width of one `HxStr`. An earlier reading placed the first tree at `+0x98`.
 * Vectors follow at `+0xac` and `+0xb8`. A nested object occupies `+0xc4` through `+0xd3`, with a
 * vector at its own `+0x00` and, following the g++ 2.x layout for a class with no base, its vptr
 * at `+0x0c`, set to `0x007e6d90`; the constructor then runs the routine at `0x001e2248` on it.
 * Words at `+0xd4`, `+0xd8`, `+0xe0`, `+0xe4`, `+0xec`, `+0xf0`, and `+0xf4` start at zero,
 * `+0xdc` starts at one, and `+0xe8` starts at -1. Two further vectors follow at `+0xf8` and
 * `+0x104`, neither of whose first word the constructor writes. A 0x38-byte record occupies
 * `+0x114` through `+0x14b`, the same record MetSaveRemix stores a vector of, with four `HxStr`
 * members built from the empty string at `0x008077d8`, a byte at `+0x134` set to one, and words at
 * `+0x138` and `+0x148`. A vector at `+0x13c` completes it.
 *
 * The constructor body is not written. The element classes of the two trees, of the five vectors,
 * of the nested object at `+0xc4`, and of the 0x38-byte record are all unidentified, so no member
 * can be declared with a type that reproduces its initialisation.
 *
 * The destructor at `0x00355070` destroys the 0x38-byte record at `+0x114`, then the vectors at
 * `+0xf8` and `+0x104`, then the nested object at `+0xc4`, then the vectors at `+0xb8` and
 * `+0xac`, then the two trees, restores the AsyncCallback vptr to `0x007f7e78` and the MemcardUser
 * vptr to `0x007daf78`, runs the MetScreen destructor, and releases the object with the tag
 * `MsgSink`. Every step is compiler-generated member destruction or a vptr restore, so the
 * definition is empty.
 */
class MetRemixManager : public MetScreen, public MemcardUser, public AsyncCallback {
public:
    /**
     * Construct the manager.
     *
     * Supplies `dlg` for the screen name, `metagame/Shared` for the directory, and `dialogue` for
     * the container. The body is not written, for the reason recorded in the class documentation.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00352b80
     */
    MetRemixManager(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x00355070
     */
    virtual ~MetRemixManager();
};
