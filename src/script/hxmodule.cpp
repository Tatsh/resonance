#include "script/hxmodule.h"

#include "script/cxx/config.h"

static const char *const kHxModuleName = "hx";

// NTSC-U/C: 0x005597b8, PAL: 0x0059a910
void InitHxModule() {
    Py_InitModule4(const_cast<char *>(kHxModuleName),
                   HxMethods().table(),
                   nullptr,
                   nullptr,
                   PYTHON_API_VERSION);
}
