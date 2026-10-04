#include "rnd/matanim.h"

#include <algorithm>
#include <iterator>
#include <list>
#include <vector>

#include "math/color.h"
#include "math/transformops.h"
#include "math/vector3.h"
#include "os/dbg.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/keychannel.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/object.h"
#include "rnd/stream.h"
#include "rnd/tex.h"
#include "rndartt/apalette.h"

namespace Rnd {

namespace {

// The revisions of the stage record, which reads the material's shared revision global. The
// three vector channels arrive from the first, and the texture channel arrives as keys rather
// than as a bare list of textures from the second.
constexpr int kStageVectorRevision = 1;
constexpr int kStageTexKeyRevision = 2;

// The revision from which Load() reads the five colour and alpha channels.
constexpr int kColorChannelRevision = 2;

// A texture read from a bare list becomes a key at its position, one frame apart.
constexpr float kLegacyTexKeySpacing = 1.0f;

// The text dump writes an absent object reference as this literal, and a present one as its
// quoted name. Both helpers below are inlined at every use in the image.
constexpr char kNoObject[] = "no object";

// The name of an object with no name of its own reads as the empty string.
const char *NameText(const Object *pObject) {
    return pObject->mName.mStr != nullptr ? pObject->mName.mStr : "";
}

void PrintObjectRef(Dbg &sink, const Object *pObject) {
    if (pObject == nullptr) {
        sink.Print(kNoObject);
        return;
    }
    sink.Format("\"%s\"", NameText(pObject));
}

void WriteObjectRef(Stream &stream, const Object *pObject) {
    if (pObject == nullptr) {
        const char chTerminator = '\0';
        stream.Write(&chTerminator, 1);
        return;
    }
    stream.Write(NameText(pObject), pObject->mName.mLen + 1);
}

template <class T>
void ReadObjectRef(Stream &stream, T *&refOut) {
    HxStr name(nullptr);
    stream.ReadString(name);
    refOut = dynamic_cast<T *>(TheManager.Find(name));
}

// Move one reference of owner from pFrom to pTo, narrowing pTo to the type of the reference.
template <class T>
void ReplaceObjectRef(Object *pOwner, T *&ref, Object *pFrom, Object *pTo) {
    if (ref == pFrom && ref != nullptr) {
        pFrom->RemoveRef(pOwner);
        ref = dynamic_cast<T *>(pTo);
        if (ref != nullptr) {
            ref->AddRef(pOwner);
        }
    }
}

// Blend a vector channel at a frame into result, and report whether the channel had a key. The
// blend runs on VU0 over three components, but the store moves the whole quadword, so the fourth
// word is the later key's. SetFrameSelf() inlines the body three times.
bool BlendChannelVector3(const std::list<Vector3Key> &keys, float flFrame, Vector3 &result) {
    if (keys.empty()) {
        return false;
    }

    const Vector3Key *pFrom = nullptr;
    const Vector3Key *pTo = nullptr;
    float flBlend = 0.0f;
    SelectKeyPair(keys, flFrame, pFrom, pTo, flBlend);

    const float flInverse = 1.0f - flBlend;
    result.x = pTo->mValue.x * flBlend + pFrom->mValue.x * flInverse;
    result.y = pTo->mValue.y * flBlend + pFrom->mValue.y * flInverse;
    result.z = pTo->mValue.z * flBlend + pFrom->mValue.z * flInverse;
    result.w = pTo->mValue.w;
    return true;
}

// Blend a colour channel at a frame into result, and report whether the channel had a key. The
// blend runs on VU0 as one quadword multiply-add. SetFrameSelf() inlines the body four times.
bool BlendChannelColor(const std::list<ColorKey> &keys, float flFrame, Color &result) {
    if (keys.empty()) {
        return false;
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
    return true;
}

// Blend a scalar channel at a frame into result, and report whether the channel had a key. Unlike
// the other blends this one is a linear step from the earlier key rather than a weighted sum.
bool BlendChannelFloat(const std::list<FloatKey> &keys, float flFrame, float &result) {
    if (keys.empty()) {
        return false;
    }

    const FloatKey *pFrom = nullptr;
    const FloatKey *pTo = nullptr;
    float flBlend = 0.0f;
    SelectKeyPair(keys, flFrame, pFrom, pTo, flBlend);

    result = (pTo->mValue - pFrom->mValue) * flBlend + pFrom->mValue;
    return true;
}

// A texture channel does not blend. The earlier key's texture holds until the next key.
// NTSC-U/C: 0x004dd550, PAL: 0x0051baf0
void InterpolateTex(Tex *pFrom,
                    [[maybe_unused]] Tex *pTo,
                    [[maybe_unused]] float flBlend,
                    Tex *&pOut) {
    pOut = pFrom;
}

// NTSC-U/C: 0x004db6b0, PAL: 0x00519bf8
Stream &ReadTexKey(Stream &stream, MatAnim::Stage::TexKey &key) {
    ReadObjectRef(stream, key.mValue);
    stream.ReadLE(&key.mFrame, sizeof(key.mFrame));
    return stream;
}

// NTSC-U/C: 0x004dda98, PAL: 0x0051c050
Stream &ReadTexKeys(Stream &stream, std::list<MatAnim::Stage::TexKey> &keys) {
    int nCount = 0;
    stream.ReadLE(&nCount, sizeof(nCount));
    keys.resize(nCount);
    for (auto &key : keys) {
        ReadTexKey(stream, key);
    }
    return stream;
}

// The texture channel as the first revision wrote it, a list of textures without frames.
// NTSC-U/C: 0x004db0c0, PAL: 0x005195d8
Stream &ReadTexList(Stream &stream, std::list<Tex *> &textures) {
    int nCount = 0;
    stream.ReadLE(&nCount, sizeof(nCount));
    textures.resize(nCount);
    for (auto &pTex : textures) {
        ReadObjectRef(stream, pTex);
    }
    return stream;
}

// NTSC-U/C: 0x004dadc8, PAL: 0x005192e0
Stream &WriteTexKeys(Stream &stream, const std::list<MatAnim::Stage::TexKey> &keys) {
    const int nCount = keys.size();
    stream.WriteLE(&nCount, sizeof(nCount));
    for (const auto &key : keys) {
        WriteObjectRef(stream, key.mValue);
        stream.WriteLE(&key.mFrame, sizeof(key.mFrame));
    }
    return stream;
}

// NTSC-U/C: 0x004daad0, PAL: 0x00518fe8
Dbg &DumpTexKeys(Dbg &sink, const std::list<MatAnim::Stage::TexKey> &keys) {
    sink.Print("(size:");
    sink.Format("%u", keys.size());
    sink.Print(")");

    unsigned nIndex = 0;
    for (const auto &key : keys) {
        sink.Print("\n");
        sink.Format("%d", nIndex);
        sink.Print("\t");
        sink.Print("(frame:");
        sink.Format("%.2f", key.mFrame);
        sink.Print(" value:");
        PrintObjectRef(sink, key.mValue);
        sink.Print(")");
        ++nIndex;
    }
    return sink;
}

// NTSC-U/C: 0x004d8b78, PAL: 0x00517090
Dbg &DumpStageAnims(Dbg &sink, std::vector<MatAnim::Stage> &stages) {
    sink.Print("(size:");
    sink.Format("%u", stages.size());
    sink.Print(")");

    int nIndex = 0;
    for (auto &stage : stages) {
        sink.Print("\n");
        sink.Format("%d", nIndex);
        sink.Print("\t");
        stage.Dump(sink);
        ++nIndex;
    }
    return sink;
}

// NTSC-U/C: 0x004d9338, PAL: 0x00517850
Stream &ReadStageAnims(Stream &stream, std::vector<MatAnim::Stage> &stages) {
    int nCount = 0;
    stream.ReadLE(&nCount, sizeof(nCount));
    stages.resize(nCount);
    for (auto &stage : stages) {
        stage.Load(stream);
    }
    return stream;
}

// NTSC-U/C: 0x004dd908, PAL: 0x0051bec0
Stream &WriteStageAnims(Stream &stream, std::vector<MatAnim::Stage> &stages) {
    const int nCount = stages.size();
    stream.WriteLE(&nCount, sizeof(nCount));
    for (auto &stage : stages) {
        stage.Save(stream);
    }
    return stream;
}

} // namespace

// NTSC-U/C: 0x00700428, PAL: 0x00743e50
HxStr MatAnim::sClassName("MatAnim");

void MatAnim::Stage::Save(Stream &stream) {
    stream << mTranslateKeys;
    stream << mScaleKeys;
    stream << mRotateKeys;
    WriteTexKeys(stream, mTexKeys);
}

void MatAnim::Stage::Load(Stream &stream) {
    if (g_nRndMatLoadVersion < kStageTexKeyRevision) {
        std::list<Tex *> textures;
        ReadTexList(stream, textures);
        mTexKeys.clear();
        float flFrame = 0.0f;
        for (const auto pTex : textures) {
            TexKey key;
            key.mValue = pTex;
            key.mFrame = flFrame;
            mTexKeys.push_back(key);
            mTexKeys.sort(); // Yes, the binary sorts the channel again after every key.
            flFrame += kLegacyTexKeySpacing;
        }
    }
    if (g_nRndMatLoadVersion >= kStageVectorRevision) {
        ReadVector3Keys(stream, mTranslateKeys);
        ReadVector3Keys(stream, mScaleKeys);
        ReadVector3Keys(stream, mRotateKeys);
    }
    if (g_nRndMatLoadVersion >= kStageTexKeyRevision) {
        ReadTexKeys(stream, mTexKeys);
    }
}

void MatAnim::Stage::Dump(Dbg &sink) {
    sink.Print(" transKeys:");
    DumpVector3Keys(sink, mTranslateKeys);
    sink.Print(" scaleKeys:");
    DumpVector3Keys(sink, mScaleKeys);
    sink.Print(" rotKeys:");
    DumpVector3Keys(sink, mRotateKeys);
    sink.Print(" texKeys:");
    DumpTexKeys(sink, mTexKeys);
    sink.Print(" matAnim:");
    PrintObjectRef(sink, mOwner);
}

void MatAnim::Stage::AddTexKey(Tex *pTex, float flFrame) {
    if (pTex != nullptr) {
        pTex->AddRef(mOwner);
    }
    TexKey key;
    key.mValue = pTex;
    key.mFrame = flFrame;
    mTexKeys.push_back(key);
    mTexKeys.sort();
}

void MatAnim::Stage::RemoveTexKey(int nIndex) {
    auto it = mTexKeys.begin();
    // Yes, the binary counts the index down to zero. A negative index walks forward.
    for (int i = nIndex; i != 0; --i) {
        ++it;
    }
    if (it->mValue != nullptr) {
        it->mValue->RemoveRef(mOwner);
    }
    mTexKeys.erase(it);
    mTexKeys.sort(); // Yes, the binary sorts a channel that erasing cannot have unsorted.
}

void MatAnim::Stage::SetTexKeyFrame(int nIndex, float flFrame) {
    auto it = mTexKeys.begin();
    std::advance(it, nIndex);
    it->mFrame = flFrame;
    mTexKeys.sort();
}

MatAnim::MatAnim(const HxStr &name) : Object(name), mMat(nullptr), mKeysOwner(this) {
}

MatAnim::~MatAnim() {
    RemoveObjectRefs();
    ReleaseAllRefs();
}

const HxStr &MatAnim::ClassName() const {
    return MatAnim::sClassName;
}

Object *CreateRegisteredMatAnim(const HxStr &name) {
    try {
        return new MatAnim(name);
    } catch (...) {
        return nullptr;
    }
}

MatAnim *NewMatAnim(const HxStr &name) {
    try {
        return new MatAnim(name);
    } catch (...) {
        return nullptr; // The binary's handler returns null.
    }
}

void RegisterMatAnimClass() {
    TheManager.RegisterClass(MatAnim::sClassName, CreateRegisteredMatAnim);
}

void MatAnim::DumpText(Dbg &sink) {
    Object::DumpText(sink);
    Animatable::DumpText(sink);
    if (sink.mDumpLevel <= 0) {
        return;
    }

    sink.Print("[MatAnim]\n");
    sink.Print("mat:");
    PrintObjectRef(sink, mMat);
    sink.Print(" stages:");
    DumpStageAnims(sink, mStages);
    sink.Print("\n");

    sink.Print("keysOwner:");
    PrintObjectRef(sink, mKeysOwner);
    sink.Print(" diffuseKeys:");
    DumpColorKeys(sink, mDiffuseKeys);
    sink.Print("\n");
    sink.Print("ambientKeys");
    DumpColorKeys(sink, mAmbientKeys);
    sink.Print("\n");
    sink.Print("emissiveKeys: ");
    DumpColorKeys(sink, mEmissiveKeys);
    sink.Print("\n");
    sink.Print("specularKeys:");
    DumpColorKeys(sink, mSpecularKeys);
    sink.Print("\n");
    sink.Print("alphaKeys:");
    DumpFloatKeys(sink, mAlphaKeys);
    sink.Print("\n");
}

void MatAnim::Replace(Object *pFrom, Object *pTo) {
    Animatable::Replace(pFrom, pTo);

    ReplaceObjectRef(this, mMat, pFrom, pTo);
    if (pTo != nullptr) {
        ReplaceObjectRef(this, mKeysOwner, pFrom, pTo);
    } else if (mKeysOwner == pFrom) {
        // Yes, the binary takes over only the stage vector of the lost owner, and it reads through
        // a null owner when pFrom is null as well.
        mStages = mKeysOwner->mStages;
        mKeysOwner = this;
    }

    for (auto &stage : mStages) {
        for (auto it = stage.mTexKeys.begin(); it != stage.mTexKeys.end();) {
            ReplaceObjectRef(this, it->mValue, pFrom, pTo);
            if (it->mValue == nullptr) {
                it = stage.mTexKeys.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void MatAnim::Load(Stream &stream) {
    // Yes, the binary reads the revision into the material's global rather than one of its own.
    stream.ReadLE(&g_nRndMatLoadVersion, sizeof(g_nRndMatLoadVersion));
    if (g_nRndMatLoadVersion > kSerialVersion) {
        Rnd::TheDbg.Notify("Can't load new MatAnim\n");
        return;
    }

    Animatable::Load(stream);
    RemoveObjectRefs();

    ReadObjectRef(stream, mMat);
    ReadStageAnims(stream, mStages);
    ReadObjectRef(stream, mKeysOwner);
    if (g_nRndMatLoadVersion >= kColorChannelRevision) {
        ReadColorKeys(stream, mDiffuseKeys);
        ReadColorKeys(stream, mAmbientKeys);
        ReadColorKeys(stream, mEmissiveKeys);
        ReadColorKeys(stream, mSpecularKeys);
        ReadFloatKeys(stream, mAlphaKeys);
    }
    if (g_nRndMatLoadVersion <= kSerialVersion) { // Yes, the test cannot fail here.
        ClearKeys();
    }

    AddObjectRefs();
}

float MatAnim::FilteredFrameEnd() {
    float flEnd = 0.0f;
    for (const auto &stage : mKeysOwner->mStages) {
        const float flTranslate = ChannelEndFrame(stage.mTranslateKeys);
        const float flScale = ChannelEndFrame(stage.mScaleKeys);
        flEnd = std::max(flEnd, std::max(flTranslate, flScale));

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

void MatAnim::SetFrameSelf(float flFrame) {
    if (mMat == nullptr) {
        return;
    }

    auto it = mKeysOwner->mStages.begin();
    for (unsigned nStage = 0; it != mKeysOwner->mStages.end() && nStage < mMat->mStages.size();
         ++it, ++nStage) {
        Mat::Stage &stage = mMat->mStages[nStage];

        BlendChannelVector3(it->mTranslateKeys, flFrame, stage.mXfm.mTranslation);

        Vector3 angles;
        angles.w = 1.0f;
        if (BlendChannelVector3(it->mRotateKeys, flFrame, angles)) {
            Rnd::MakeRotMatrix(&angles.x, &stage.mXfm.mBasisX.x);
        }

        Vector3 scale;
        scale.w = 1.0f;
        if (BlendChannelVector3(it->mScaleKeys, flFrame, scale)) {
            Rnd::Scale(&scale.x, &stage.mXfm.mBasisX.x, &stage.mXfm.mBasisX.x);
        }

        if (!it->mTexKeys.empty()) {
            const Stage::TexKey *pFrom = nullptr;
            const Stage::TexKey *pTo = nullptr;
            float flBlend = 0.0f;
            SelectKeyPair(it->mTexKeys, flFrame, pFrom, pTo, flBlend);
            Tex *pTex = nullptr;
            InterpolateTex(pFrom->mValue, pTo->mValue, flBlend, pTex);
            stage.SetTex(pTex);
        }
    }

    Color color;
    if (BlendChannelColor(mKeysOwner->mDiffuseKeys, flFrame, color)) {
        mMat->SetDiffuse(color);
    }
    if (BlendChannelColor(mKeysOwner->mAmbientKeys, flFrame, color)) {
        mMat->SetAmbient(color);
    }
    if (BlendChannelColor(mKeysOwner->mEmissiveKeys, flFrame, color)) {
        mMat->SetEmissive(color);
    }
    if (BlendChannelColor(mKeysOwner->mSpecularKeys, flFrame, color)) {
        mMat->SetSpecular(color, 0.0f);
    }
    float flAlpha = 0.0f;
    if (BlendChannelFloat(mKeysOwner->mAlphaKeys, flFrame, flAlpha)) {
        mMat->SetAlpha(flAlpha);
    }
}

void MatAnim::Save(Stream &stream) {
    const int nVersion = kSerialVersion;
    stream.WriteLE(&nVersion, sizeof(nVersion));

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

void MatAnim::SetMat(Mat *pMat) {
    if (mMat != nullptr) {
        mMat->RemoveRef(this);
    }
    mMat = pMat;
    if (pMat != nullptr) {
        pMat->AddRef(this);
    }
}

void MatAnim::SetKeysOwner(MatAnim *pOwner) {
    if (mKeysOwner != nullptr) {
        mKeysOwner->RemoveRef(this);
    }
    mKeysOwner = pOwner;
    if (pOwner != nullptr) {
        pOwner->AddRef(this);
    }
    ClearKeys();
}

void MatAnim::SetNumStages(int nCount) {
    std::vector<Stage> &stages = mKeysOwner->mStages;
    if (static_cast<unsigned>(nCount) < stages.size()) {
        for (auto it = stages.begin() + nCount; it != stages.end(); ++it) {
            for (const auto &key : it->mTexKeys) {
                if (key.mValue != nullptr) {
                    key.mValue->RemoveRef(this);
                }
            }
        }
    }
    stages.resize(nCount);
    for (auto &stage : stages) {
        stage.mOwner = this;
    }
}

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
