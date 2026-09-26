#include "app/apptunnel.h"
#include "app/playsound.h"
#include "app/renderer.h"
#include "app/tunnelcache.h"
#include "rnd/tunnel.h"
#include "script/cxx/config.h"
#include "script/cxx/object.h"
#include "script/cxx/tuple.h"

namespace {

// Flip the LSD filter.
//
// Only while a renderer exists. The tuple arrives by value and is released here.
// 0x0042d558
Py::Object ScriptLsdMode([[maybe_unused]] Py::Tuple args) {
    if (g_pRenderer != nullptr) {
        g_nLsdMode ^= 1;
        PlayActivateSound();
    }
    return Py::Object();
}

// Run ScriptLsdMode() on the interpreter's argument tuple.
// 0x0042d670
PyObject *PyInvokeLsdMode(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptLsdMode(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Flip the crate gems.
//
// Only while an app tunnel exists. The tuple arrives by value and is released here.
// 0x00449dc0
Py::Object ScriptCrates([[maybe_unused]] Py::Tuple args) {
    if (g_pAppTunnel != nullptr) {
        g_pAppTunnel->mShowCrates ^= 1;
        PlayActivateSound();
    }
    return Py::Object();
}

// Run ScriptCrates() on the interpreter's argument tuple.
// 0x00449ed0
PyObject *PyInvokeCrates(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptCrates(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Cycle the lattice and panel meshes through their three visibility states.
//
// Both drawn, panels only, neither, and back to both. A missing tunnel draws nothing and
// returns at once; the sound plays only while an app tunnel exists. The tuple arrives by value
// and is released here.
// 0x0044a080
Py::Object ScriptNolatticeToggle([[maybe_unused]] Py::Tuple args) {
    Rnd::Tunnel *pTunnel = GetCachedTunnelObject();
    if (pTunnel == nullptr) {
        return Py::Object();
    }
    if (g_pAppTunnel != nullptr) {
        PlayActivateSound();
    }
    if (pTunnel->mDrawLattice != 0) {
        if (pTunnel->mDrawPanels != 0) {
            pTunnel->mDrawLattice = 0;
        } else {
            pTunnel->mDrawLattice = 1;
            pTunnel->mDrawPanels = 1;
        }
    } else if (pTunnel->mDrawPanels != 0) {
        pTunnel->mDrawPanels = 0;
    } else {
        pTunnel->mDrawLattice = 1;
        pTunnel->mDrawPanels = 1;
    }
    return Py::Object();
}

// Run ScriptNolatticeToggle() on the interpreter's argument tuple.
// 0x0044a1d0
PyObject *PyInvokeNolatticeToggle(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptNolatticeToggle(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

} // namespace
