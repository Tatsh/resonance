#pragma once

#include "game/rawcontroller.h"

/**
 * Owner of the front-end world, outside a game session.
 *
 * `13MetaGameWorld` in the RTTI descriptor at `0x00901c90`, with RawController as its one public
 * base at offset 0. The object is 0xc bytes, which the allocation in GameManagerImpl::Start()
 * fixes, and its vtable is at `0x00810f58` with three entries, the type function, the destructor at
 * `0x003d4790`, and one further virtual at `0x003d3288`.
 *
 * Recovery has barely started. This declaration exists so that GameManagerImpl can type the world
 * it creates in Start() and vends through vtable slot 16.
 *
 * Two of the three members are recorded by address rather than declared. `0x003d4858` returns the
 * pointer at `+0x04`, which is a polymorphic object of its own, and `0x003d4890` runs slot 5 of
 * that object. GameManagerImpl drives the second one from three of its message handlers.
 *
 * The constructor stores a null in `+0x04` and installs an 8-byte object in `+0x08` through
 * `0x001daaf8`, whose class is not recovered.
 */
class MetaGameWorld : public RawController {
public:
    /**
     * @ghidraAddress 0x003d3110
     */
    MetaGameWorld();

    /**
     * @ghidraAddress 0x003d4790
     */
    virtual ~MetaGameWorld();
};
