#include "script/scriptfunc.h"

#include "script/hxmodule.h"

// 0x00559720
ScriptFunc::ScriptFunc(const char *pszName, PyCFunction pfnMethod) {
    HxMethods().add(pszName, pfnMethod);
}
