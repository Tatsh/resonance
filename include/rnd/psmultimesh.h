#pragma once

#include "os/hxstr.h"
#include "rnd/multimesh.h"

namespace Rnd {

/**
 * PlayStation 2 multi-mesh, which submits one GIF packet run per instance.
 *
 * `Q23Rnd11PsMultiMesh` in the RTTI descriptor at `0x008ef490`, with `Rnd::MultiMesh` as its one
 * public base at offset 0. The class declares no data member, so it is the same 0x38 bytes the
 * base occupies and the creator at `0x005b5c28` allocates exactly that. Its two vtables at
 * `0x00833d80` and `0x00833dc8` differ from the Rnd::MultiMesh pair only in the type info
 * accessor, the destructor, and DrawSelf().
 */
class PsMultiMesh : public MultiMesh {
public:
    /**
     * Construct an empty PlayStation 2 multi-mesh.
     *
     * The body is empty beyond the base construction. Rnd::NewPsMultiMesh() is the only
     * construction site.
     *
     * @param name The object name, passed to the Rnd::Object constructor.
     * @ghidraAddress 0x005b2fd8
     */
    PsMultiMesh(const HxStr &name);

    /** @ghidraAddress 0x005b5a40 */
    virtual ~PsMultiMesh();

protected:
    /**
     * Draw every instance through the vector unit path.
     *
     * Rnd::Drawable vtable slot 3. Falls back on Rnd::MultiMesh::DrawSelf() while the device
     * submits geometry through the software path. An empty transform list, an absent mesh, and a
     * mesh with no faces all yield without drawing. Otherwise the material passes run in a loop,
     * each one selecting the depth registers and the material and then submitting the whole
     * instance run.
     *
     * @return Non-zero, always, so the children are drawn as well.
     * @ghidraAddress 0x005b2ed0
     */
    virtual int DrawSelf();

    /**
     * Submit the GIF packets for every instance of the current material pass.
     *
     * The body is not reconstructed.
     *
     * @ghidraAddress 0x005b2c60
     */
    void SubmitInstanceGifPackets();
};

/**
 * Allocate and construct a PlayStation 2 multi-mesh.
 *
 * GfxDevice::Init() installs this creator in Rnd::g_pfnNewMultiMesh at `0x0049af88`, so a
 * multi-mesh loaded from a file on the PlayStation 2 is a PsMultiMesh.
 *
 * @param name The object name.
 * @return The new multi-mesh.
 * @ghidraAddress 0x005b5c28
 */
MultiMesh *NewPsMultiMesh(const HxStr &name);

} // namespace Rnd
