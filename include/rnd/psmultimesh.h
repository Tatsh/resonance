#pragma once

#include "rnd/multimesh.h"

namespace Rnd {

/**
 * PlayStation 2 multi-mesh, which submits one GIF packet run per instance.
 *
 * `Q23Rnd11PsMultiMesh` in the RTTI descriptor at `0x008ef490`, with `Rnd::MultiMesh` as its one
 * public base at offset 0.
 *
 * Recovery is partial. The routines recovered so far are the constructor, the destructor, the
 * class factory and its registration, the instance packet submission at `0x005b2c60`, and the
 * instanced draw at `0x005b2ed0`. None of their bodies is reconstructed.
 */
class PsMultiMesh : public MultiMesh {
public:
    /**
     * Construct an empty PlayStation 2 multi-mesh.
     *
     * @param name The object name, passed to the Rnd::Object constructor.
     * @ghidraAddress 0x005b2fd8
     */
    PsMultiMesh(const HxStr &name);

    /** @ghidraAddress 0x005b5a40 */
    virtual ~PsMultiMesh();

    /**
     * Submit the GIF packets for every instance.
     *
     * @ghidraAddress 0x005b2c60
     */
    void SubmitInstanceGifPackets();
};

/**
 * Register the PlayStation 2 multi-mesh creator with Rnd::Manager.
 *
 * @ghidraAddress 0x005b5be8
 */
void RegisterPsMultiMeshClass();

/**
 * Allocate and construct a PlayStation 2 multi-mesh.
 *
 * @param name The object name.
 * @return The new multi-mesh.
 * @ghidraAddress 0x005b5c28
 */
MultiMesh *NewPsMultiMesh(const HxStr &name);

} // namespace Rnd
