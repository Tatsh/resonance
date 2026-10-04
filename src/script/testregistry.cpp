#include "script/testregistry.h"

#include <string.h>

// NTSC-U/C: 0x008ef950, PAL: 0x00934950
TestRegistry::Entry TestRegistry::sTests[kMaxTests];

// NTSC-U/C: 0x0086f790, PAL: 0x008b3e70
int TestRegistry::sTestCount;

void TestRegistry::Register(const char *pszName, TestFunc pfnTest) {
    sTests[sTestCount].mName = pszName;
    sTests[sTestCount].mFunc = pfnTest;
    ++sTestCount;
}

TestRegistry::TestFunc TestRegistry::Find(const char *pszName) {
    for (int i = 0; i < sTestCount; ++i) {
        if (strcmp(sTests[i].mName, pszName) == 0) {
            return sTests[i].mFunc;
        }
    }
    return nullptr;
}
