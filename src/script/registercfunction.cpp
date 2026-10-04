#include "script/registercfunction.h"

#include "script/hxmodule.h"

RegisterCFunction::RegisterCFunction(const char *pszName, PyCFunction pfnMethod) {
    HxMethods().add(pszName, pfnMethod);
}
