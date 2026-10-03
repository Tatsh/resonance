#include "script/cxx/string.h"

#include "script/cxx/fromapi.h"

namespace Py {

// NTSC-U/C: 0x004c4d88, PAL: 0x00503010
String::String(const HxStr &text)
    : SeqBase<Char>(
          FromAPI(PyString_FromString(text.mStr != nullptr ? text.mStr : g_szEmptyString)).mPtr) {
    validate();
}

// NTSC-U/C: 0x004c5138, PAL: 0x005033c0
String::String(const char *pszText)
    : SeqBase<Char>(FromAPI(PyString_FromString(const_cast<char *>(pszText))).mPtr) {
    validate();
}

// NTSC-U/C: 0x004c4c60, PAL: 0x00502ee8
String::String(const Object &ob) : SeqBase<Char>(ob) {
    validate();
}

} // namespace Py
