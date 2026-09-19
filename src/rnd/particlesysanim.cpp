#include "rnd/particlesysanim.h"

#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/keychannel.h"
#include "rnd/object.h"
#include "rnd/particlesys.h"

namespace Rnd {

namespace {

// The text dump writes an absent object reference as this literal, and a present one as its
// quoted name. src/rnd/mesh.cpp declares the same pair file-locally for the same reason.
constexpr char kNoObject[] = "no object";

// The name of an object with no name of its own reads as the empty string.
const char *NameText(const Object *pObject) {
    return pObject->mName.mStr != nullptr ? pObject->mName.mStr : "";
}

void PrintObjectRef(FailSink &sink, const Object *pObject) {
    if (pObject == nullptr) {
        sink.Print(kNoObject);
        return;
    }
    sink.Format("\"%s\"", NameText(pObject));
}

} // namespace

// 0x0071af00
HxStr g_particleSysAnimClassName("ParticleSysAnim");

// 0x0052bc90
const HxStr &ParticleSysAnim::ClassName() const {
    return g_particleSysAnimClassName;
}

// 0x00526980
void ParticleSysAnim::DumpText(FailSink &sink) {
    Object::DumpText(sink);
    Animatable::DumpText(sink);
    if (sink.mDumpLevel <= 0) {
        return;
    }

    sink.Print("[ParticleSysAnim]\n");
    sink.Print("particleSys:");
    PrintObjectRef(sink, mParticleSys);
    sink.Print(" framesOwner:");
    PrintObjectRef(sink, mFramesOwner);
    sink.Print(" emitRateRatio:");
    sink.Format("%.2f", mEmitRateRatio);
    sink.Print("\n");
    sink.Print("startColorKeys:");
    DumpColorKeys(sink, mStartColorKeys);
    sink.Print("\n");
    sink.Print("endColorKeys:");
    DumpColorKeys(sink, mEndColorKeys);
    sink.Print("\n");
    sink.Print("emitRateKeys:");
    DumpFloatKeys(sink, mEmitRateKeys);
    sink.Print("\n");
}

// 0x0052c180
ParticleSysAnim *NewParticleSysAnim(const HxStr &name) {
    // The allocation is untagged here, unlike every other renderer class, and it is exactly 0x4c
    // bytes rather than a rounded size.
    return new ParticleSysAnim(name);
}

} // namespace Rnd
