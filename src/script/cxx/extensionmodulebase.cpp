#include "script/cxx/extensionmodulebase.h"

#include "script/cxx/extensionmodulebaseptr.h"

namespace Py {

ExtensionModuleBase::ExtensionModuleBase(const char *pszName) : mModuleName(pszName) {
}

ExtensionModuleBase::~ExtensionModuleBase() {
}

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
