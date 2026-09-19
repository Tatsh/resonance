#pragma once

#include "met/metrenderer.h"
#include "rnd/mesh.h"

/**
 * Driver of the full-screen metagame fade.
 *
 * The class emits no RTTI descriptor and no `__FILE__` path survives for its translation unit,
 * because its constructor writes no virtual function table pointer and it declares no destructor.
 * The name here is inferred from the two literals its constructor uses, `metfade.rect` at
 * `0x007d8dd0` and `met_fade.view` alongside it.
 *
 * The object is 0x2c bytes, which the `MemAllocScalar(0x2c)` at both of its two allocation sites
 * fixes. MetLoadGameScreen and MetMemDetectStartup each build one and each release it through the
 * scalar deallocator with no null test, which is what a delete expression compiles to for a class
 * with no destructor.
 *
 * This declaration is partial. The constructor settles five offsets. The remaining words at
 * `+0x04`, `+0x0c` through `+0x18`, and `+0x24` are never written by it, so their purpose and
 * width are undetermined and they are recorded as reserved spans. No member name is attested
 * anywhere in the image, so every identifier below follows the required style.
 */
class MetFade {
public:
    /**
     * Resolve the fade rectangle and prime the two timers.
     *
     * Both timers start at 1.0e9, which is the sentinel for a fade that is not running. The
     * rectangle is the object named `metfade.rect`, cast to Rnd::Mesh, and a null result is stored
     * as such. The constructor then invokes slot 1 of the mesh table on the resolved rectangle
     * with a zero argument.
     *
     * @param pRenderer The front-end renderer the fade draws through.
     * @ghidraAddress 0x0016a1c8
     */
    MetFade(MetRenderer *pRenderer);

private:
    float unknown00_; // +0x00, starts at 1.0e9
    int unknown04_;   // +0x04, not written by the constructor
    float unknown08_; // +0x08, starts at 1.0e9
    // Not written by the constructor and not recovered. +0x0c
    unsigned char unknown0c_[0x10];
    Rnd::Mesh *rect_;       // +0x1c, the object named `metfade.rect`
    int unknown20_;         // +0x20
    int unknown24_;         // +0x24, not written by the constructor
    MetRenderer *renderer_; // +0x28
};
