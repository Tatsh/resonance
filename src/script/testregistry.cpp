#include "script/testregistry.h"

#include <string.h>

// 0x008ef950
TestRegistry::Entry TestRegistry::sTests[kMaxTests];

// 0x0086f790
int TestRegistry::sTestCount;

// 0x0015fda8
void TestRegistry::Register(const char *pszName, TestFunc pfnTest) {
    sTests[sTestCount].mName = pszName;
    sTests[sTestCount].mFunc = pfnTest;
    ++sTestCount;
}

// 0x0015fdd8
TestRegistry::TestFunc TestRegistry::Find(const char *pszName) {
    for (int i = 0; i < sTestCount; ++i) {
        if (strcmp(sTests[i].mName, pszName) == 0) {
            return sTests[i].mFunc;
        }
    }
    return nullptr;
}
