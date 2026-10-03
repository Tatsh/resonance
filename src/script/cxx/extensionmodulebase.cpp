#include "script/cxx/extensionmodulebase.h"

#include "script/cxx/extensionmodulebaseptr.h"

namespace Py {

// NTSC-U/C: 0x005a5d28, PAL: 0x005e8218
ExtensionModuleBase::ExtensionModuleBase(const char *pszName) : mModuleName(pszName) {
}

// NTSC-U/C: 0x005a5d98, PAL: 0x005e8298
ExtensionModuleBase::~ExtensionModuleBase() {
}

// NTSC-U/C: 0x005a5ea0, PAL: 0x005e83b0
void ExtensionModuleBase::initialize(const char *pszModuleDoc) {
    PyObject *pModulePtr = new ExtensionModuleBasePtr(this);
    Py_InitModule4(
        const_cast<char *>(mModuleName.mStr != nullptr ? mModuleName.mStr : g_szEmptyString),
        mMethodTable.table(),
        const_cast<char *>(pszModuleDoc),
        pModulePtr,
        PYTHON_API_VERSION);
}

} // namespace Py
