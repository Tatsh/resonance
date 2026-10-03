#include <exception>

#include "app/apptunnel.h"
#include "app/playsound.h"
#include "app/renderer.h"
#include "app/tunnelcache.h"
#include "rnd/tunnel.h"
#include "script/cxx/config.h"
#include "script/cxx/object.h"
#include "script/cxx/tuple.h"
#include "script/registercfunction.h"

namespace {

// Flip the LSD filter.
//
// Only while a renderer exists. The tuple arrives by value and is released here.
// NTSC-U/C: 0x0042d558, PAL: 0x00469110
Py::Object ScriptLsdMode([[maybe_unused]] Py::Tuple args) {
    if (g_pRenderer != nullptr) {
        g_nLsdMode ^= 1;
        PlayActivateSound();
    }
    return Py::Object();
}

// Run ScriptLsdMode() on the interpreter's argument tuple.
// NTSC-U/C: 0x0042d670, PAL: 0x00469228
PyObject *PyInvokeLsdMode(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptLsdMode(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Flip the crate gems.
//
// Only while an app tunnel exists. The tuple arrives by value and is released here.
// NTSC-U/C: 0x00449dc0, PAL: 0x004870b0
Py::Object ScriptCrates([[maybe_unused]] Py::Tuple args) {
    if (g_pAppTunnel != nullptr) {
        g_pAppTunnel->mShowCrates ^= 1;
        PlayActivateSound();
    }
    return Py::Object();
}

// Run ScriptCrates() on the interpreter's argument tuple.
// NTSC-U/C: 0x00449ed0, PAL: 0x004871c0
PyObject *PyInvokeCrates(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptCrates(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Cycle the lattice and panel meshes through their three visibility states.
//
// Both drawn, panels only, neither, and back to both. The mode does not change and no sound plays
// unless both the tunnel and the app tunnel exist. The tuple arrives by value and is released here.
// NTSC-U/C: 0x0044a080, PAL: 0x00487370
Py::Object ScriptNolatticeToggle([[maybe_unused]] Py::Tuple args) {
    Rnd::Tunnel *pTunnel = GetCachedTunnelObject();
    if (pTunnel == nullptr || g_pAppTunnel == nullptr) {
        return Py::Object();
    }
    PlayActivateSound();
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
// NTSC-U/C: 0x0044a1d0, PAL: 0x004874c0
PyObject *PyInvokeNolatticeToggle(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptNolatticeToggle(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// The script interface this file exports, registered in static initialisation.
// NTSC-U/C: 0x00431ab8, PAL: 0x0046d728
const RegisterCFunction kLsdmodeFunc("lsdmode", PyInvokeLsdMode);
// NTSC-U/C: 0x00453bc8, PAL: 0x004910c8
const RegisterCFunction kCratesFunc("crates", PyInvokeCrates);
const RegisterCFunction kNolatticeFunc("nolattice", PyInvokeNolatticeToggle);

} // namespace
