#include "rnd/manager.h"

#include <map>

#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/object.h"

namespace Rnd {

namespace {

// Dump level at which the objects the renderer created itself are written as well.
constexpr int kInternalObjectDumpLevel = 2;

// 0x0051f798
// Writes the class registry as its entry count and then one indented line per entry.
// The factory is reported as a truth value rather than as an address, which is what the two
// literals "true" and "false" at 0x00826e20 and 0x00826e28 are for. Only DumpText() calls it.
FailSink *DumpRegisteredClasses(FailSink *pSink, const std::map<HxStr, ClassFactory> &classes) {
    pSink->Print("(size:")->Format("%u", classes.size())->Print(")");

    for (const auto &entry : classes) {
        pSink->Print("\n\t")
            ->Print("key:")
            ->Format("\"%s\"", entry.first.mStr != nullptr ? entry.first.mStr : g_szEmptyString)
            ->Print(" value:")
            ->Print(entry.second != nullptr ? "true" : "false");
    }

    return pSink;
}

} // namespace

// 0x0051ad98
void Manager::DumpText(FailSink &sink) {
    sink.Print("[Manager]\n");
    DumpRegisteredClasses(sink.Print("registeredClasses:"), mClasses)->Print("\n");

    sink.Print("objects:\n\n");
    for (const auto &entry : mObjects) {
        if (entry.second->mInternal == 0) {
            entry.second->DumpText(sink);
            sink.Print("\n");
        }
    }

    if (sink.mDumpLevel < kInternalObjectDumpLevel) {
        return;
    }

    sink.Print("internalObjects:\n\n");
    for (const auto &entry : mObjects) {
        if (entry.second->mInternal != 0) {
            entry.second->DumpText(sink);
            sink.Print("\n");
        }
    }
}

// 0x0051bf70
void Manager::DeleteLoadedObjects() {
    for (;;) {
        auto entry = mObjects.begin();
        while (entry != mObjects.end() && entry->second->mInternal != 0) {
            ++entry;
        }
        if (entry == mObjects.end()) {
            return;
        }
        // An entry whose object is null spins here rather than making progress, because the
        // destroy is what would have erased it.
        delete entry->second;
    }
}

} // namespace Rnd
