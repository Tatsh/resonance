#include "rnd/tunnel.h"

#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/drawable.h"
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

// 0x00476288
Tunnel *NewTunnel(const HxStr &name) {
    // The binary bills the allocation to the tag "Rnd::Tunnel" and the object is 0x104 bytes.
    return new Tunnel(name);
}

} // namespace Rnd
