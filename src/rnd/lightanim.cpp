#include "rnd/lightanim.h"

#include <algorithm>
#include <list>

#include "math/color.h"
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

void WriteObjectRef(Stream &stream, const Object *pObject) {
    if (pObject == nullptr) {
        const char chTerminator = '\0';
        stream.WriteBytes(&chTerminator, 1);
        return;
    }
    stream.WriteBytes(NameText(pObject), pObject->mName.mLen + 1);
}

// Replace a colour from a channel at a frame, retaining it while the channel is empty. The blend
// runs on VU0 as one quadword multiply-add, which is why the image addresses both keyframes as
// whole elements rather than loading the components. SetFrameSelf() inlines the body three times.
void BlendChannelColor(const std::list<ColorKey> &keys, float flFrame, Color &result) {
    if (keys.empty()) {
        return;
    }

    const ColorKey *pFrom = nullptr;
    const ColorKey *pTo = nullptr;
    float flBlend = 0.0f;
    SelectKeyPair(keys, flFrame, pFrom, pTo, flBlend);

    const float flInverse = 1.0f - flBlend;
    result.r = pTo->mValue.r * flBlend + pFrom->mValue.r * flInverse;
    result.g = pTo->mValue.g * flBlend + pFrom->mValue.g * flInverse;
    result.b = pTo->mValue.b * flBlend + pFrom->mValue.b * flInverse;
    result.a = pTo->mValue.a * flBlend + pFrom->mValue.a * flInverse;
}

} // namespace

// 0x00720bd8
HxStr g_lightAnimClassName("LightAnim");

// 0x00544de0
const HxStr &LightAnim::ClassName() const {
    return g_lightAnimClassName;
}

// 0x00544dd0
Light *LightAnim::GetLight() {
    return mLight;
}

// 0x00544dd8
LightAnim *LightAnim::GetKeysOwner() {
    return mKeysOwner;
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
    // `0x004d9aa0`.
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

// 0x00541790
float LightAnim::EndFrame() {
    const float flAmbient = ChannelEndFrame(mKeysOwner->mAmbientKeys);
    const float flDiffuse = ChannelEndFrame(mKeysOwner->mDiffuseKeys);
    const float flSpecular = ChannelEndFrame(mKeysOwner->mSpecularKeys);
    return std::max(flAmbient, std::max(flDiffuse, flSpecular));
}

// 0x005418f8
void LightAnim::SetFrameSelf(float flFrame) {
    if (mLight == nullptr) {
        return;
    }

    Color ambient = mLight->mAmbient;
    Color diffuse = mLight->mDiffuse;
    Color specular = mLight->mSpecular;

    BlendChannelColor(mKeysOwner->mAmbientKeys, flFrame, ambient);
    BlendChannelColor(mKeysOwner->mDiffuseKeys, flFrame, diffuse);
    BlendChannelColor(mKeysOwner->mSpecularKeys, flFrame, specular);

    mLight->SetColors(ambient, diffuse, specular);
}

// 0x00541230
void LightAnim::Save(Stream &stream) {
    const int nVersion = kSerialVersion;
    stream.Write(&nVersion, sizeof(nVersion));

    Animatable::Save(stream);

    WriteObjectRef(stream, mLight);
    WriteColorKeys(stream, mAmbientKeys);
    WriteColorKeys(stream, mDiffuseKeys);
    WriteColorKeys(stream, mSpecularKeys);
    WriteObjectRef(stream, mKeysOwner);
}

// 0x00541638
void LightAnim::Copy(const Object *pSource, unsigned nFlags) {
    const LightAnim *pSourceAnim = dynamic_cast<const LightAnim *>(pSource);

    Animatable::Copy(pSource, nFlags);

    if (mLight != nullptr) {
        mLight->RemoveRef(this);
    }
    if (mKeysOwner != nullptr) {
        mKeysOwner->RemoveRef(this);
    }

    mLight = pSourceAnim->mLight;
    if ((nFlags & kCopyShareKeys) != 0 || pSourceAnim->mKeysOwner != pSourceAnim) {
        mKeysOwner = pSourceAnim->mKeysOwner;
        if (mKeysOwner != this) {
            mAmbientKeys.clear();
            mDiffuseKeys.clear();
            mSpecularKeys.clear();
        }
    } else {
        mKeysOwner = this;
        mAmbientKeys = pSourceAnim->mAmbientKeys;
        mDiffuseKeys = pSourceAnim->mDiffuseKeys;
        mSpecularKeys = pSourceAnim->mSpecularKeys;
    }

    if (mLight != nullptr) {
        mLight->AddRef(this);
    }
    if (mKeysOwner != nullptr) {
        mKeysOwner->AddRef(this);
    }
}

// 0x00541388
void LightAnim::Load(Stream &stream) {
    int nRevision = 0;
    stream.Read(&nRevision, sizeof(nRevision));
    if (nRevision > kSerialVersion) {
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
    if (nRevision <= kSerialVersion && mKeysOwner != this) {
        mAmbientKeys.clear();
        mDiffuseKeys.clear();
        mSpecularKeys.clear();
    }

    if (mLight != nullptr) {
        mLight->AddRef(this);
    }
    if (mKeysOwner != nullptr) {
        mKeysOwner->AddRef(this);
    }
}

// 0x005449f8
LightAnim *NewLightAnim(const HxStr &name) {
    // The allocation is untagged here, as it is for Rnd::ParticleSysAnim, and it is exactly 0x48
    // bytes rather than a rounded size.
    try {
        return new LightAnim(name);
    } catch (...) {
        return nullptr; // The binary's handler returns null.
    }
}

// 0x005452d0
Object *CreateRegisteredLightAnim(const HxStr &name) {
    return NewLightAnim(name);
}

// 0x005449c8
void RegisterLightAnimClass() {
    g_manager.RegisterClass(g_lightAnimClassName, CreateRegisteredLightAnim);
}

// 0x00545570
void LightAnim::ClearKeys() {
    if (mKeysOwner == this) {
        return;
    }
    mAmbientKeys.clear();
    mDiffuseKeys.clear();
    mSpecularKeys.clear();
}

// 0x005455b8
void LightAnim::SetKeysOwner(LightAnim *pOwner) {
    if (mKeysOwner != nullptr) {
        mKeysOwner->RemoveRef(this);
    }
    mKeysOwner = pOwner;
    if (pOwner != nullptr) {
        pOwner->AddRef(this);
    }
    ClearKeys();
}

} // namespace Rnd
