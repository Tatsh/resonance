#include "rnd/psenviron.h"

#include "os/hxstr.h"
#include "rnd/environ.h"
#include "rnd/light.h"
#include "rnd/pscam.h"

namespace Rnd {

// 0x0077613c
PsEnviron *g_pDefaultEnviron;

// 0x00776140
Light *g_pDefaultLight;

// 0x00776110
float g_flFogScale;

// 0x00776114
float g_flFogOffset;

// 0x005b2200
PsEnviron::~PsEnviron() {
}

// 0x005b27b0
Environ *PsEnviron::NewEnviron(const HxStr &name) {
    // The binary bills the allocation to the tag "Rnd::Environ" and the object is 0x80 bytes.
    return new PsEnviron(name);
}

// 0x005aea68
void PsEnviron::Init() {
    g_pfnNewEnviron = NewEnviron;
    g_pDefaultEnviron = new PsEnviron(HxStr("[default environ]"));
    g_pDefaultEnviron->mInternal = 1;
    g_pDefaultCam->AddDraw(g_pDefaultEnviron, nullptr);

    g_pDefaultLight = g_pfnNewLight(HxStr("[default light]"));
    g_pDefaultLight->mInternal = 1;
    g_pDefaultEnviron->AddLight(g_pDefaultLight);
    g_pDefaultCam->AddTrans(g_pDefaultLight);
}

} // namespace Rnd
