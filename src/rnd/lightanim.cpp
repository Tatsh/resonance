#include "rnd/lightanim.h"

#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/keychannel.h"
#include "rnd/light.h"
#include "rnd/manager.h"
#include "rnd/object.h"
#include "rnd/stream.h"

namespace Rnd {

namespace {

// Revision Save() writes and the highest revision Load() accepts.
constexpr int kLightAnimRevision = 0;

// The text dump writes an absent object reference as this literal, and a present one as its
// quoted name. src/rnd/mesh.cpp and src/rnd/particlesysanim.cpp declare the same pair
// file-locally for the same reason.
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

// 0x00720bd8
HxStr g_lightAnimClassName("LightAnim");

// 0x00544de0
const HxStr &LightAnim::ClassName() const {
    return g_lightAnimClassName;
}

// 0x00541078
void LightAnim::DumpText(FailSink &sink) {
    Object::DumpText(sink);
    Animatable::DumpText(sink);
    if (sink.mDumpLevel <= 0) {
        return;
    }

    sink.Print("[LightAnim]\n");
    sink.Print("light:");
    PrintObjectRef(sink, mLight);
    sink.Print(" keysOwner:");
    PrintObjectRef(sink, mKeysOwner);
    sink.Print("\n");
    sink.Print("ambientKeys:");
    DumpColorKeys(sink, mAmbientKeys);
    sink.Print("\n");
    sink.Print("diffuseKeys:");
    DumpColorKeys(sink, mDiffuseKeys);
    sink.Print("\n");
    sink.Print("specularKeys:");
    DumpColorKeys(sink, mSpecularKeys);
    sink.Print("\n");
}

// 0x00540ea0
void LightAnim::Replace(Object *pFrom, Object *pTo) {
    Animatable::Replace(pFrom, pTo);

    if (mLight == pFrom && mLight != nullptr) {
        pFrom->RemoveRef(this);
        mLight = dynamic_cast<Light *>(pTo);
        if (mLight != nullptr) {
            mLight->AddRef(this);
        }
    }

    // Losing the animation that owned the shared keys takes the three channels over rather than
    // dropping them. Each of the three assignments is the `std::list` copy assignment at
    // `0x004d9aa0`, which the one-word channel model in the header cannot express as a call.
    if (pTo != nullptr) {
        if (mKeysOwner == pFrom && mKeysOwner != nullptr) {
            pFrom->RemoveRef(this);
            mKeysOwner = dynamic_cast<LightAnim *>(pTo);
            if (mKeysOwner != nullptr) {
                mKeysOwner->AddRef(this);
            }
        }
    } else if (mKeysOwner == pFrom && mKeysOwner != nullptr) {
        mAmbientKeys = mKeysOwner->mAmbientKeys;
        mDiffuseKeys = mKeysOwner->mDiffuseKeys;
        mSpecularKeys = mKeysOwner->mSpecularKeys;
        mKeysOwner = this;
    }
}

// 0x00541388
void LightAnim::Load(Stream &stream) {
    int nRevision = 0;
    stream.Read(&nRevision, sizeof(nRevision));
    if (nRevision > kLightAnimRevision) {
        g_failSink.Report("Can't load new LightAnim\n");
        return;
    }

    Animatable::Load(stream);

    if (mLight != nullptr) {
        mLight->RemoveRef(this);
    }
    if (mKeysOwner != nullptr) {
        mKeysOwner->RemoveRef(this);
    }

    HxStr lightName(nullptr);
    stream.ReadString(lightName);
    mLight = dynamic_cast<Light *>(g_manager.Find(lightName));

    ReadColorKeys(stream, mAmbientKeys);
    ReadColorKeys(stream, mDiffuseKeys);
    ReadColorKeys(stream, mSpecularKeys);

    HxStr ownerName(nullptr);
    stream.ReadString(ownerName);
    mKeysOwner = dynamic_cast<LightAnim *>(g_manager.Find(ownerName));

    // The revision test is redundant, because a revision above 0 has already returned above. The
    // binary tests it a second time regardless.
    if (nRevision <= kLightAnimRevision && mKeysOwner != this) {
        ClearColorKeys(mAmbientKeys);
        ClearColorKeys(mDiffuseKeys);
        ClearColorKeys(mSpecularKeys);
    }

    if (mLight != nullptr) {
        mLight->AddRef(this);
    }
    if (mKeysOwner != nullptr) {
        mKeysOwner->AddRef(this);
    }
}

// 0x005452d0
LightAnim *NewLightAnim(const HxStr &name) {
    // The allocation is untagged here, as it is for Rnd::ParticleSysAnim, and it is exactly 0x48
    // bytes rather than a rounded size.
    return new LightAnim(name);
}

} // namespace Rnd
