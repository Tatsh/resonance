#include "rnd/tunnel.h"

#include <vector>

#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/drawable.h"
#include "rnd/mesh.h"
#include "rnd/object.h"

namespace Rnd {

// 0x006eab10
HxStr g_tunnelClassName("Tunnel");

// 0x004763b8
const HxStr &Tunnel::ClassName() const {
    return g_tunnelClassName;
}

// 0x004768b8
void Tunnel::DumpText(FailSink &sink) {
    Object::DumpText(sink);
    Drawable::DumpText(sink);
    Animatable::DumpText(sink);
    if (sink.mDumpLevel <= 0) {
        return;
    }

    // The author never finished this block. It writes no member of the class, and the Collideable
    // base is not dumped either.
    sink.Print("[Tunnel]\n");
    sink.Print("TODO\n");
}

// 0x0046d180
void Tunnel::ApplyMeshLodScreenSizes(const std::vector<float> &screenSizes) {
    mUnknown68 = screenSizes;
    for (unsigned nSlice = 0; nSlice < mUnknowna4.size(); ++nSlice) {
        for (unsigned nRing = 0; nRing < mUnknowna4[nSlice].size(); ++nRing) {
            if (nRing < mUnknown68.size()) {
                Mesh *pMesh = mUnknowna4[nSlice][nRing];
                pMesh->mMinScreen = mUnknown68[nRing];
                // Yes, the binary releases and immediately re-takes the reference on the same
                // link, because the argument is the link the mesh already stores.
                pMesh->SetNext(pMesh->mNext);
            }
        }
    }
    for (unsigned nSlice = 0; nSlice < mUnknownb0.size(); ++nSlice) {
        for (unsigned nRing = 0; nRing < mUnknownb0[nSlice].size(); ++nRing) {
            if (nRing < mUnknown68.size()) {
                Mesh *pMesh = mUnknownb0[nSlice][nRing];
                pMesh->mMinScreen = mUnknown68[nRing];
                pMesh->SetNext(pMesh->mNext);
            }
        }
    }
}

// 0x00476288
Tunnel *NewTunnel(const HxStr &name) {
    // The binary bills the allocation to the tag "Rnd::Tunnel" and the object is 0x104 bytes.
    return new Tunnel(name);
}

} // namespace Rnd
