#include "script/scriptfunc.h"

#include "script/hxmodule.h"

// NTSC-U/C: 0x00559720, PAL: 0x0059a878
ScriptFunc::ScriptFunc(const char *pszName, PyCFunction pfnMethod) {
    HxMethods().add(pszName, pfnMethod);
}
