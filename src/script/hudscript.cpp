#include <exception>

#include "app/hudanimramp.h"
#include "app/hudpanel.h"
#include "app/linearramp.h"
#include "app/overlay.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "script/cxx/config.h"
#include "script/cxx/float.h"
#include "script/cxx/int.h"
#include "script/cxx/object.h"
#include "script/cxx/string.h"
#include "script/cxx/tuple.h"
#include "script/cxx/typeerror.h"
#include "script/scriptfunc.h"

namespace {

// Move the activator label to a value.
// NTSC-U/C: 0x00420d00, PAL: 0x0045c148
Py::Object ScriptActivatorLabel(Py::Tuple args) {
    if (g_pOverlay == nullptr) {
        return Py::Object();
    }
    if (args.length() != 1) {
        throw Py::TypeError(HxStr("wrong # args for activator_label"));
    }
    Py::Float number(Py::FromAPI(PyNumber_Float(args.getItem(0).mPtr)).mPtr);
    const float flValue = static_cast<float>(PyFloat_AsDouble(number.mPtr));
    g_pOverlay->mPanel->mLabelSwap.mRamp.SetTarget(flValue);
    return Py::Object();
}

// Put the highlight box on a rectangle at once.
// NTSC-U/C: 0x00421238, PAL: 0x0045c6a0
Py::Object ScriptHighlightSnap(Py::Tuple args) {
    if (g_pOverlay == nullptr) {
        return Py::Object();
    }
    if (args.length() != 4) {
        throw Py::TypeError(HxStr("wrong # args for highlight_snap"));
    }
    Py::Float leftNumber(Py::FromAPI(PyNumber_Float(args.getItem(0).mPtr)).mPtr);
    const float flLeft = static_cast<float>(PyFloat_AsDouble(leftNumber.mPtr));
    Py::Float topNumber(Py::FromAPI(PyNumber_Float(args.getItem(1).mPtr)).mPtr);
    const float flTop = static_cast<float>(PyFloat_AsDouble(topNumber.mPtr));
    Py::Float rightNumber(Py::FromAPI(PyNumber_Float(args.getItem(2).mPtr)).mPtr);
    const float flRight = static_cast<float>(PyFloat_AsDouble(rightNumber.mPtr));
    Py::Float bottomNumber(Py::FromAPI(PyNumber_Float(args.getItem(3).mPtr)).mPtr);
    const float flBottom = static_cast<float>(PyFloat_AsDouble(bottomNumber.mPtr));
    g_pOverlay->mPanel->mHighlight.JumpTo(flLeft, flTop, flRight, flBottom);
    return Py::Object();
}

// Glide the highlight box to a rectangle over a time.
// NTSC-U/C: 0x004220f8, PAL: 0x0045d580
Py::Object ScriptHighlightSlide(Py::Tuple args) {
    if (g_pOverlay == nullptr) {
        return Py::Object();
    }
    if (args.length() != 5) {
        throw Py::TypeError(HxStr("wrong # args for highlight_slide"));
    }
    Py::Float leftNumber(Py::FromAPI(PyNumber_Float(args.getItem(0).mPtr)).mPtr);
    const float flLeft = static_cast<float>(PyFloat_AsDouble(leftNumber.mPtr));
    Py::Float topNumber(Py::FromAPI(PyNumber_Float(args.getItem(1).mPtr)).mPtr);
    const float flTop = static_cast<float>(PyFloat_AsDouble(topNumber.mPtr));
    Py::Float rightNumber(Py::FromAPI(PyNumber_Float(args.getItem(2).mPtr)).mPtr);
    const float flRight = static_cast<float>(PyFloat_AsDouble(rightNumber.mPtr));
    Py::Float bottomNumber(Py::FromAPI(PyNumber_Float(args.getItem(3).mPtr)).mPtr);
    const float flBottom = static_cast<float>(PyFloat_AsDouble(bottomNumber.mPtr));
    Py::Float durationNumber(Py::FromAPI(PyNumber_Float(args.getItem(4).mPtr)).mPtr);
    const float flDuration = static_cast<float>(PyFloat_AsDouble(durationNumber.mPtr));
    g_pOverlay->mPanel->mHighlight.MoveTo(flLeft, flTop, flRight, flBottom, flDuration);
    return Py::Object();
}

// Show or hide the highlight box.
//
// Only a value of 1 shows.
// NTSC-U/C: 0x004232d8, PAL: 0x0045e780
Py::Object ScriptHighlightSetshow(Py::Tuple args) {
    if (g_pOverlay == nullptr) {
        return Py::Object();
    }
    if (args.length() != 1) {
        throw Py::TypeError(HxStr("wrong # args for highlight_show"));
    }
    const long long nShow = Py::Int(args.getItem(0));
    g_pOverlay->mPanel->mHighlight.SetShowing(nShow == 1 ? 1 : 0);
    return Py::Object();
}

// Show or hide the analog stick prompt.
//
// Only a value of 1 shows.
// NTSC-U/C: 0x004236f8, PAL: 0x0045ebc0
Py::Object ScriptAnalogStickSetshow(Py::Tuple args) {
    if (g_pOverlay == nullptr) {
        return Py::Object();
    }
    if (args.length() != 1) {
        throw Py::TypeError(HxStr("wrong # args for analog_stick_show"));
    }
    const long long nShow = Py::Int(args.getItem(0));
    g_pOverlay->mPanel->mAnalogStick.SetShowing(nShow == 1 ? 1 : 0);
    return Py::Object();
}

// Show or hide the controller group.
//
// Only a value of 1 shows. The length error repeats the analog stick message, as the image
// does.
// NTSC-U/C: 0x00423b18, PAL: 0x0045f000
Py::Object ScriptControllerSetshow(Py::Tuple args) {
    if (g_pOverlay == nullptr) {
        return Py::Object();
    }
    if (args.length() != 1) {
        throw Py::TypeError(HxStr("wrong # args for analog_stick_show"));
    }
    const long long nShow = Py::Int(args.getItem(0));
    g_pOverlay->mPanel->mTcGroup.SetShowing(nShow == 1 ? 1 : 0);
    return Py::Object();
}

// Put the material for one stick motion on the prompt.
//
// The binary expands the two branches of HudAnalogStick::SetMotion() inline; calling it repeats
// them without duplicating the body.
// NTSC-U/C: 0x00423f40, PAL: 0x0045f448
Py::Object ScriptAnalogStickSetmat(Py::Tuple args) {
    if (g_pOverlay == nullptr) {
        return Py::Object();
    }
    if (args.length() != 1) {
        throw Py::TypeError(HxStr("wrong # args for analog_stick_show"));
    }
    Py::Object element = args.getItem(0);
    Py::String text(element);
    HxStr motion = text;
    g_pOverlay->mPanel->mAnalogStick.SetMotion(motion);
    return Py::Object();
}

// Run ScriptActivatorLabel() on the interpreter's argument tuple.
// NTSC-U/C: 0x004243c0, PAL: 0x0045f908
PyObject *PyInvokeActivatorLabel(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptActivatorLabel(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Run ScriptHighlightSnap() on the interpreter's argument tuple.
// NTSC-U/C: 0x00424570, PAL: 0x0045fab8
PyObject *PyInvokeHighlightSnap(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptHighlightSnap(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Run ScriptHighlightSlide() on the interpreter's argument tuple.
// NTSC-U/C: 0x00424720, PAL: 0x0045fc68
PyObject *PyInvokeHighlightSlide(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptHighlightSlide(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Run ScriptHighlightSetshow() on the interpreter's argument tuple.
// NTSC-U/C: 0x004248d0, PAL: 0x0045fe18
PyObject *PyInvokeHighlightSetshow(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptHighlightSetshow(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Run ScriptAnalogStickSetshow() on the interpreter's argument tuple.
// NTSC-U/C: 0x00424a80, PAL: 0x0045ffc8
PyObject *PyInvokeAnalogStickSetshow(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptAnalogStickSetshow(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Run ScriptControllerSetshow() on the interpreter's argument tuple.
// NTSC-U/C: 0x00424c30, PAL: 0x00460178
PyObject *PyInvokeControllerSetshow(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptControllerSetshow(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Run ScriptAnalogStickSetmat() on the interpreter's argument tuple.
// NTSC-U/C: 0x00424de0, PAL: 0x00460328
PyObject *PyInvokeAnalogStickSetmat(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptAnalogStickSetmat(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// The script interface this file exports, registered in static initialisation.
// NTSC-U/C: 0x00429348, PAL: 0x00464950
const ScriptFunc kActivatorLabelFunc("activator_label", PyInvokeActivatorLabel);
const ScriptFunc kHighlightSnapFunc("highlight_snap", PyInvokeHighlightSnap);
const ScriptFunc kHighlightSlideFunc("highlight_slide", PyInvokeHighlightSlide);
const ScriptFunc kHighlightSetshowFunc("highlight_setshow", PyInvokeHighlightSetshow);
const ScriptFunc kAnalogStickSetshowFunc("analog_stick_setshow", PyInvokeAnalogStickSetshow);
const ScriptFunc kControllerSetshowFunc("controller_setshow", PyInvokeControllerSetshow);
const ScriptFunc kAnalogStickSetmatFunc("analog_stick_setmat", PyInvokeAnalogStickSetmat);

} // namespace
