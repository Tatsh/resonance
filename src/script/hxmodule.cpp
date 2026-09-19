#include "script/hxmodule.h"

#include "script/cxx/config.h"

static const char *const kHxModuleName = "hx";

// 0x005597b8
void InitHxModule() {
    Py_InitModule4(const_cast<char *>(kHxModuleName),
                   HxMethods().table(),
                   nullptr,
                   nullptr,
                   PYTHON_API_VERSION);
}
