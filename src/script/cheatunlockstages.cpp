#include <exception>

#include "app/application.h"
#include "game/metagameworld.h"
#include "met/metrenderer.h"
#include "script/cxx/config.h"
#include "script/cxx/object.h"
#include "script/cxx/tuple.h"
#include "script/registercfunction.h"

namespace {

// Unlock every stage.
// NTSC-U/C: 0x001506c0, PAL: 0x00151410
// PyInvokeCheatUnlockstages() expands this inline, and the out-of-line copy has no caller.
inline Py::Object ScriptActivateAllAccessMode([[maybe_unused]] const Py::Tuple &args) {
    MetRenderer *pRenderer =
        dynamic_cast<MetRenderer *>(Application::shared()->GetMetaWorld()->GetRenderer());
    if (pRenderer != nullptr) {
        pRenderer->UnlockAllStages();
    }
    return Py::Object();
}

// Run ScriptActivateAllAccessMode() on the interpreter's argument tuple.
// NTSC-U/C: 0x0014f0c8, PAL: 0x0014fda8
PyObject *PyInvokeCheatUnlockstages(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptActivateAllAccessMode(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// The script interface this file exports, registered in static initialisation.
// NTSC-U/C: 0x00150580, PAL: 0x001512d0
const RegisterCFunction kCheatUnlockstagesFunc("cheat_unlockstages", PyInvokeCheatUnlockstages);

} // namespace
