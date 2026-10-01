#include "script/cxx/string.h"

#include "script/cxx/fromapi.h"

namespace Py {

// 0x004c4d88
String::String(const HxStr &text)
    : SeqBase<Char>(
          FromAPI(PyString_FromString(text.mStr != nullptr ? text.mStr : g_szEmptyString)).mPtr) {
    validate();
}

// 0x004c5138
String::String(const char *pszText)
    : SeqBase<Char>(FromAPI(PyString_FromString(const_cast<char *>(pszText))).mPtr) {
    validate();
}

// 0x004c4c60
String::String(const Object &ob) : SeqBase<Char>(ob) {
    validate();
}

} // namespace Py
