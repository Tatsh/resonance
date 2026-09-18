#pragma once

#include <list>

#include "math/transform.h"
#include "os/failsink.h"
#include "rnd/drawable.h"
#include "rnd/mesh.h"
#include "rnd/stream.h"

namespace Rnd {

/**
 * Draws one mesh many times, once per recorded instance transform.
 *
 * `Q23Rnd9MultiMesh` in the RTTI descriptor at `0x008ef150`, with `Rnd::Drawable` as its one
 * public base at offset 0. The Drawable subobject is 0x14 bytes and the shared Rnd::Object
 * subobject sits at `+0x1c`, so the two members below occupy the gap between them.
 *
 * Recovery is partial. Two words of the instance record are undetermined, and the hardware
 * submission path is not reconstructed at all; it belongs to Rnd::PsMultiMesh.
 */
class MultiMesh : public Drawable {
public:
    /**
     * One placement of the shared mesh.
     *
     * The record is 0x50 bytes. Its two leading words are undetermined, and the member order
     * follows the order the instance text dump writes, a float, an integer, and then the four
     * transform rows.
     */
    struct Instance {
        float mUnknown00; // +0x00
        int mUnknown04;   // +0x04
        Transform mXfm;   // +0x10
    };

    /**
     * Construct an empty multi-mesh.
     *
     * @param name The object name, passed to the Rnd::Object constructor.
     * @ghidraAddress 0x004e8830
     */
    MultiMesh(const HxStr &name);

    /** @ghidraAddress 0x004e85c0 */
    virtual ~MultiMesh();

    /** @ghidraAddress 0x004e81a0 */
    virtual void DumpText(FailSink &sink);

    /** @ghidraAddress 0x004e8288 */
    virtual void Load(Stream &stream);

    /**
     * Add this multi-mesh to the collision hit list of its mesh.
     *
     * @ghidraAddress 0x004ebde0
     */
    void AddToHitList();

    /**
     * Remove this multi-mesh from the collision hit list of its mesh.
     *
     * @ghidraAddress 0x004ebe10
     */
    void RemoveFromHitList();

protected:
    /**
     * Draw the mesh once per instance.
     *
     * @return Non-zero when the children are to be drawn as well.
     * @ghidraAddress 0x004e83d0
     */
    virtual int DrawSelf();

    // Both members are protected because Rnd::PsMultiMesh reads the mesh and walks the instance
    // list to submit its GIF packets. The order below is the recovered offset order.
    Mesh *mMesh;                    // +0x14
    std::list<Instance> mInstances; // +0x18
};

} // namespace Rnd
