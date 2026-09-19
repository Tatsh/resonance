#include "rnd/matanim.h"

#include <algorithm>
#include <list>
#include <vector>

#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/keychannel.h"
#include "rnd/mat.h"
#include "rnd/object.h"
#include "rnd/stream.h"

namespace Rnd {

namespace {

// The name of an object with no name of its own reads as the empty string.
const char *NameText(const Object *pObject) {
    return pObject->mName.mStr != nullptr ? pObject->mName.mStr : "";
}

void WriteObjectRef(Stream &stream, const Object *pObject) {
    if (pObject == nullptr) {
        const char chTerminator = '\0';
        stream.WriteBytes(&chTerminator, 1);
        return;
    }
    stream.WriteBytes(NameText(pObject), pObject->mName.mLen + 1);
}

// 0x004dd908
Stream &WriteStageAnims(Stream &stream, std::vector<MatAnim::StageAnim> &stages) {
    const int nCount = stages.size();
    stream.Write(&nCount, sizeof(nCount));
    for (auto &stage : stages) {
        stage.Save(stream);
    }
    return stream;
}

} // namespace

// 0x00700428
HxStr g_matAnimClassName("MatAnim");

// 0x004dc4e0
const HxStr &MatAnim::ClassName() const {
    return g_matAnimClassName;
}

// 0x004dcb00
Object *CreateRegisteredMatAnim(const HxStr &name) {
    return new MatAnim(name);
}

// 0x004d42f0
float MatAnim::EndFrame() {
    float flEnd = 0.0f;
    for (const auto &stage : mKeysOwner->mStages) {
        const float flTranslate = ChannelEndFrame(stage.mTranslateKeys);
        const float flXfm = ChannelEndFrame(stage.mXfmKeys);
        flEnd = std::max(flEnd, std::max(flTranslate, flXfm));

        const float flRotate = ChannelEndFrame(stage.mRotateKeys);
        const float flTex = ChannelEndFrame(stage.mTexKeys);
        flEnd = std::max(flEnd, std::max(flRotate, flTex));
    }

    const float flDiffuse = ChannelEndFrame(mKeysOwner->mDiffuseKeys);
    const float flAmbient = ChannelEndFrame(mKeysOwner->mAmbientKeys);
    flEnd = std::max(flEnd, std::max(flDiffuse, flAmbient));

    const float flEmissive = ChannelEndFrame(mKeysOwner->mEmissiveKeys);
    const float flSpecular = ChannelEndFrame(mKeysOwner->mSpecularKeys);
    flEnd = std::max(flEnd, std::max(flEmissive, flSpecular));

    const float flAlpha = ChannelEndFrame(mKeysOwner->mAlphaKeys);
    return std::max(flEnd, flAlpha);
}

// 0x004d35d0
void MatAnim::Save(Stream &stream) {
    const int nVersion = kSerialVersion;
    stream.Write(&nVersion, sizeof(nVersion));

    Animatable::Save(stream);

    WriteObjectRef(stream, mMat);
    WriteStageAnims(stream, mStages);
    WriteObjectRef(stream, mKeysOwner);
    WriteColorKeys(stream, mDiffuseKeys);
    WriteColorKeys(stream, mAmbientKeys);
    WriteColorKeys(stream, mEmissiveKeys);
    WriteColorKeys(stream, mSpecularKeys);
    WriteFloatKeys(stream, mAlphaKeys);
}

// 0x004dd388
void MatAnim::Copy(const Object *pSource, unsigned nFlags) {
    const MatAnim *pSourceAnim = dynamic_cast<const MatAnim *>(pSource);

    Animatable::Copy(pSource, nFlags);
    RemoveObjectRefs();

    mMat = pSourceAnim->mMat;
    if ((nFlags & kCopyShareKeys) != 0 || pSourceAnim->mKeysOwner != pSourceAnim) {
        mKeysOwner = pSourceAnim->mKeysOwner;
        ClearKeys();
    } else {
        mKeysOwner = this;
        mStages = pSourceAnim->mStages;
        mDiffuseKeys = pSourceAnim->mDiffuseKeys;
        mAmbientKeys = pSourceAnim->mAmbientKeys;
        mEmissiveKeys = pSourceAnim->mEmissiveKeys;
        mSpecularKeys = pSourceAnim->mSpecularKeys;
        mAlphaKeys = pSourceAnim->mAlphaKeys;
    }

    AddObjectRefs();
}

// 0x004d2fe0
void MatAnim::ClearKeys() {
    if (mKeysOwner == this) {
        return;
    }

    mStages.clear();
    mDiffuseKeys.clear();
    mAmbientKeys.clear();
    mEmissiveKeys.clear();
    mSpecularKeys.clear();
    mAlphaKeys.clear();
}

// 0x004d3818
void MatAnim::AddObjectRefs() {
    if (mMat != nullptr) {
        mMat->AddRef(this);
    }
    if (mKeysOwner != nullptr) {
        mKeysOwner->AddRef(this);
    }
    for (auto &stage : mStages) {
        stage.mOwner = this;
        for (const auto &key : stage.mTexKeys) {
            if (key.mValue != nullptr) {
                key.mValue->AddRef(this);
            }
        }
    }
}

// 0x004d3750
void MatAnim::RemoveObjectRefs() {
    if (mMat != nullptr) {
        mMat->RemoveRef(this);
    }
    if (mKeysOwner != nullptr) {
        mKeysOwner->RemoveRef(this);
    }
    for (const auto &stage : mStages) {
        for (const auto &key : stage.mTexKeys) {
            if (key.mValue != nullptr) {
                key.mValue->RemoveRef(this);
            }
        }
    }
}

} // namespace Rnd
