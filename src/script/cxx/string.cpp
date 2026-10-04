#include "script/cxx/string.h"

#include "script/cxx/fromapi.h"

namespace Py {

String::String(const HxStr &text)
    : SeqBase<Char>(
          FromAPI(PyString_FromString(text.mStr != nullptr ? text.mStr : g_szEmptyString)).mPtr) {
    validate();
}

String::String(const char *pszText)
    : SeqBase<Char>(FromAPI(PyString_FromString(const_cast<char *>(pszText))).mPtr) {
    validate();
}

String::String(const Object &ob) : SeqBase<Char>(ob) {
    validate();
}

} // namespace Py
