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
#include "script/cxx/int.h"
#include "script/cxx/object.h"
#include "script/cxx/string.h"
#include "script/cxx/tuple.h"
#include "script/cxx/typeerror.h"

namespace {

// Show or report a drawable.
//
// An empty tuple reports the showing flag. Otherwise the first element selects showing, and a
// value above zero shows. The tuple arrives by value, so the copy the caller builds is released
// here.
// 0x0044a380
Py::Object ShowDrawable(Py::Tuple args, Rnd::Drawable *pTarget) {
    if (pTarget == nullptr) {
        return Py::Object();
    }
    if (args.length() == 0) {
        return Py::Int(static_cast<long>(pTarget->mShowing));
    }
    const long nShow = Py::Int(args.getItem(0));
    pTarget->SetShowing(nShow > 0 ? 1 : 0);
    return Py::Object();
}

// Show the activator.
// 0x0044a7b0
Py::Object ScriptActivatorShow(const Py::Tuple &args) {
    Rnd::Drawable *pTarget =
        dynamic_cast<Rnd::Drawable *>(Rnd::g_manager.Find(HxStr("activator0")));
    return ShowDrawable(Py::Tuple(args), pTarget);
}

// Show the now ring.
// 0x0044a930
Py::Object ScriptNowRing(const Py::Tuple &args) {
    Rnd::Drawable *pTarget =
        dynamic_cast<Rnd::Drawable *>(Rnd::g_manager.Find(HxStr("nowring.view")));
    return ShowDrawable(Py::Tuple(args), pTarget);
}

// Show the three head-up displays.
// 0x0044aab0
Py::Object ScriptHudEnable(const Py::Tuple &args) {
    Rnd::Drawable *pEnergy =
        dynamic_cast<Rnd::Drawable *>(Rnd::g_manager.Find(HxStr("HUD1 energy.mesh")));
    ShowDrawable(Py::Tuple(args), pEnergy);
    Rnd::Drawable *pScore =
        dynamic_cast<Rnd::Drawable *>(Rnd::g_manager.Find(HxStr("HUD1 score0.mesh")));
    ShowDrawable(Py::Tuple(args), pScore);
    Rnd::Drawable *pPos =
        dynamic_cast<Rnd::Drawable *>(Rnd::g_manager.Find(HxStr("HUD1 pos.view")));
    ShowDrawable(Py::Tuple(args), pPos);
    return Py::Object();
}

// Show the section boundary.
// 0x0044af40
Py::Object ScriptSections(const Py::Tuple &args) {
    Rnd::Drawable *pMessage =
        dynamic_cast<Rnd::Drawable *>(Rnd::g_manager.Find(HxStr("boundary msg")));
    ShowDrawable(Py::Tuple(args), pMessage);
    Rnd::Drawable *pRing =
        dynamic_cast<Rnd::Drawable *>(Rnd::g_manager.Find(HxStr("boundary_ring.mesh")));
    ShowDrawable(Py::Tuple(args), pRing);
    return Py::Object();
}

// Show or hide the first player's seeker.
//
// An empty tuple reports the seeker fade's active flag. Otherwise the first element selects
// showing: a value above zero applies the fade's stored range to the seeker, anything else
// applies an empty range. Either way the sabre-trail string follows the flag. Without an app
// tunnel there is nothing to drive. The tuple arrives by value and is released here.
// 0x0044b2d8
Py::Object ScriptSeeker(Py::Tuple args) {
    if (g_pAppTunnel == nullptr) {
        return Py::Object();
    }
    TnlPlayer *pPlayer = g_pAppTunnel->mPlayers[0];
    if (args.length() == 0) {
        return Py::Int(static_cast<long>(pPlayer->mSeekerFade.mActive));
    }
    const long nShow = Py::Int(args.getItem(0));
    const int bShow = nShow > 0 ? 1 : 0;
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
// 0x0044b7a0
Py::Object ScriptFadeActivator(const Py::Tuple &args) {
    Rnd::Mat *pActTarUp = dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(HxStr("act_g tar up.mat")));
    Rnd::Mat *pScratch = dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(HxStr("scratch_g.mat")));
    Rnd::Mat *pAct = dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(HxStr("act_g.mat")));
    Rnd::Mat *pPtrAxe = dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(HxStr("ptr_g_axe.mat")));
    Rnd::Mat *pScratchPlate =
        dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(HxStr("scratchplate_g.mat")));
    if (pActTarUp == nullptr || pScratch == nullptr || pAct == nullptr || pPtrAxe == nullptr ||
        pScratchPlate == nullptr) {
        return Py::Object();
    }
    if (args.length() != 1) {
        throw Py::TypeError(HxStr(FormatString("wrong # args for fade_activator")));
    }
    Py::Object number(PyNumber_Float(args.getItem(0).mPtr));
    const float flAlpha = static_cast<float>(PyFloat_AsDouble(number.mPtr));
    pActTarUp->SetAlpha(flAlpha);
    pAct->SetAlpha(flAlpha);
    pScratch->SetAlpha(flAlpha);
    pPtrAxe->SetAlpha(flAlpha);
    pScratchPlate->SetAlpha(flAlpha);
    return Py::Object();
}

// Run ScriptActivatorShow() on the interpreter's argument tuple.
// 0x0044c080
PyObject *PyInvokeActivatorShow(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptActivatorShow(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Run ScriptNowRing() on the interpreter's argument tuple.
// 0x0044c230
PyObject *PyInvokeNowRing(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptNowRing(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Run ScriptHudEnable() on the interpreter's argument tuple.
// 0x0044c8f0
PyObject *PyInvokeHudEnable(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptHudEnable(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Run ScriptSections() on the interpreter's argument tuple.
// 0x0044c3e0
PyObject *PyInvokeSections(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptSections(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Run ScriptSeeker() on the interpreter's argument tuple.
// 0x0044c590
PyObject *PyInvokeSeeker(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptSeeker(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Run ScriptFadeActivator() on the interpreter's argument tuple.
// 0x0044c740
PyObject *PyInvokeFadeActivator(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptFadeActivator(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

} // namespace
