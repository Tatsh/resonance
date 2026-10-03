#include <exception>

#include "app/apptunnel.h"
#include "app/tnlplayer.h"
#include "app/tnlsabretrail.h"
#include "app/tnlseekerfade.h"
#include "app/tunnelcache.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/drawable.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/string.h"
#include "rnd/tunnel.h"
#include "rnd/tunnelseeker.h"
#include "script/cxx/config.h"
#include "script/cxx/float.h"
#include "script/cxx/int.h"
#include "script/cxx/object.h"
#include "script/cxx/string.h"
#include "script/cxx/tuple.h"
#include "script/cxx/typeerror.h"
#include "script/registercfunction.h"

namespace {

// Show or report a drawable.
//
// An empty tuple reports the showing flag. Otherwise the first element selects showing, and any
// nonzero value shows. The tuple arrives by value, and the copy the caller builds is released
// here.
// NTSC-U/C: 0x0044a380, PAL: 0x00487670
Py::Object ShowDrawable(Py::Tuple args, Rnd::Drawable *pTarget) {
    if (pTarget == nullptr) {
        return Py::Object();
    }
    if (args.length() == 0) {
        return Py::Int(static_cast<long long>(pTarget->mShowing));
    }
    const int nShow = Py::Int(args.getItem(0));
    pTarget->SetShowing(nShow != 0 ? 1 : 0);
    return Py::Object();
}

// Show the activator.
// NTSC-U/C: 0x0044a7b0, PAL: 0x00487aa0
Py::Object ScriptActivatorShow(Py::Tuple args) {
    Rnd::Drawable *pTarget =
        dynamic_cast<Rnd::Drawable *>(Rnd::TheManager.Find(HxStr("activator0")));
    return ShowDrawable(Py::Tuple(args), pTarget);
}

// Show the now ring.
// NTSC-U/C: 0x0044a930, PAL: 0x00487c40
Py::Object ScriptNowRing(Py::Tuple args) {
    Rnd::Drawable *pTarget =
        dynamic_cast<Rnd::Drawable *>(Rnd::TheManager.Find(HxStr("nowring.view")));
    return ShowDrawable(Py::Tuple(args), pTarget);
}

// Show the three head-up displays.
// NTSC-U/C: 0x0044aab0, PAL: 0x00487de0
Py::Object ScriptHudEnable(Py::Tuple args) {
    Rnd::Drawable *pEnergy =
        dynamic_cast<Rnd::Drawable *>(Rnd::TheManager.Find(HxStr("HUD1 energy.mesh")));
    ShowDrawable(Py::Tuple(args), pEnergy);
    Rnd::Drawable *pScore =
        dynamic_cast<Rnd::Drawable *>(Rnd::TheManager.Find(HxStr("HUD1 score0.mesh")));
    ShowDrawable(Py::Tuple(args), pScore);
    Rnd::Drawable *pPos =
        dynamic_cast<Rnd::Drawable *>(Rnd::TheManager.Find(HxStr("HUD1 pos.view")));
    ShowDrawable(Py::Tuple(args), pPos);
    return Py::Object();
}

// Show the section boundary.
// NTSC-U/C: 0x0044af40, PAL: 0x004882e8
Py::Object ScriptSections(Py::Tuple args) {
    Rnd::Drawable *pMessage =
        dynamic_cast<Rnd::Drawable *>(Rnd::TheManager.Find(HxStr("boundary msg")));
    ShowDrawable(Py::Tuple(args), pMessage);
    Rnd::Drawable *pRing =
        dynamic_cast<Rnd::Drawable *>(Rnd::TheManager.Find(HxStr("boundary_ring.mesh")));
    ShowDrawable(Py::Tuple(args), pRing);
    return Py::Object();
}

// Show or hide the first player's seeker.
//
// An empty tuple reports the seeker fade's active flag. Otherwise the first element selects
// showing. Any nonzero value applies the fade's stored range to the seeker, and anything else
// applies an empty range. Either way the sabre-trail string follows the flag. Without an app
// tunnel there is nothing to drive. The tuple arrives by value and is released here.
// NTSC-U/C: 0x0044b2d8, PAL: 0x004886e0
Py::Object ScriptSeeker(Py::Tuple args) {
    if (g_pAppTunnel == nullptr) {
        return Py::Object();
    }
    TnlPlayer *pPlayer = g_pAppTunnel->mPlayers[0];
    if (args.length() == 0) {
        return Py::Int(static_cast<long long>(pPlayer->mSeekerFade.mActive));
    }
    const int nShow = Py::Int(args.getItem(0));
    const int bShow = nShow != 0 ? 1 : 0;
    pPlayer->mSeekerFade.mActive = bShow;
    Rnd::TunnelSeeker *pSeeker = GetCachedTunnelObject()->GetSeeker(pPlayer->mSeekerFade.mIndex);
    if (bShow == 0) {
        pSeeker->SetRange(0, 0, 0);
    } else {
        pSeeker->SetRange(pPlayer->mSeekerFade.mFirstSlice,
                          pPlayer->mSeekerFade.mSliceCount,
                          pPlayer->mSeekerFade.mRing);
    }
    pPlayer->mSabreTrail.mString->SetShowing(bShow);
    return Py::Object();
}

// Fade the activator materials to one alpha.
//
// All five materials have to resolve, or nothing happens. The tuple carries the alpha.
// NTSC-U/C: 0x0044b7a0, PAL: 0x00488ba8
Py::Object ScriptFadeActivator(Py::Tuple args) {
    Rnd::Mat *pActTarUp = dynamic_cast<Rnd::Mat *>(Rnd::TheManager.Find(HxStr("act_g tar up.mat")));
    Rnd::Mat *pScratch = dynamic_cast<Rnd::Mat *>(Rnd::TheManager.Find(HxStr("scratch_g.mat")));
    Rnd::Mat *pAct = dynamic_cast<Rnd::Mat *>(Rnd::TheManager.Find(HxStr("act_g.mat")));
    Rnd::Mat *pPtrAxe = dynamic_cast<Rnd::Mat *>(Rnd::TheManager.Find(HxStr("ptr_g_axe.mat")));
    Rnd::Mat *pScratchPlate =
        dynamic_cast<Rnd::Mat *>(Rnd::TheManager.Find(HxStr("scratchplate_g.mat")));
    if (pActTarUp == nullptr || pScratch == nullptr || pAct == nullptr || pPtrAxe == nullptr ||
        pScratchPlate == nullptr) {
        return Py::Object();
    }
    if (args.length() != 1) {
        throw Py::TypeError(HxStr("wrong # args for fade_activator"));
    }
    Py::Float number(Py::FromAPI(PyNumber_Float(args.getItem(0).mPtr)).mPtr);
    const float flAlpha = static_cast<float>(PyFloat_AsDouble(number.mPtr));
    pActTarUp->SetAlpha(flAlpha);
    pAct->SetAlpha(flAlpha);
    pScratch->SetAlpha(flAlpha);
    pPtrAxe->SetAlpha(flAlpha);
    pScratchPlate->SetAlpha(flAlpha);
    return Py::Object();
}

// Run ScriptActivatorShow() on the interpreter's argument tuple.
// NTSC-U/C: 0x0044c080, PAL: 0x00489538
PyObject *PyInvokeActivatorShow(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptActivatorShow(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Run ScriptNowRing() on the interpreter's argument tuple.
// NTSC-U/C: 0x0044c230, PAL: 0x004896e8
PyObject *PyInvokeNowRing(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptNowRing(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Run ScriptHudEnable() on the interpreter's argument tuple.
// NTSC-U/C: 0x0044c8f0, PAL: 0x00489da8
PyObject *PyInvokeHudEnable(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptHudEnable(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Run ScriptSections() on the interpreter's argument tuple.
// NTSC-U/C: 0x0044c3e0, PAL: 0x00489898
PyObject *PyInvokeSections(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptSections(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Run ScriptSeeker() on the interpreter's argument tuple.
// NTSC-U/C: 0x0044c590, PAL: 0x00489a48
PyObject *PyInvokeSeeker(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptSeeker(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Run ScriptFadeActivator() on the interpreter's argument tuple.
// NTSC-U/C: 0x0044c740, PAL: 0x00489bf8
PyObject *PyInvokeFadeActivator(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptFadeActivator(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// The script interface this file exports, registered in static initialisation.
// NTSC-U/C: 0x00453bc8, PAL: 0x004910c8
const RegisterCFunction kActivatorFunc("activator", PyInvokeActivatorShow);
const RegisterCFunction kNowringFunc("nowring", PyInvokeNowRing);
const RegisterCFunction kSectionsFunc("sections", PyInvokeSections);
const RegisterCFunction kSeekerFunc("seeker", PyInvokeSeeker);
const RegisterCFunction kFadeActivatorFunc("fade_activator", PyInvokeFadeActivator);
const RegisterCFunction kHudEnableFunc("hud_enable", PyInvokeHudEnable);

} // namespace
