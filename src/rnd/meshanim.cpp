#include "rnd/meshanim.h"

#include <algorithm>
#include <list>
#include <vector>

#include "math/color.h"
#include "math/vector2.h"
#include "math/vector3.h"
#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mesh.h"
#include "rnd/meshvert.h"
#include "rnd/stream.h"

namespace Rnd {

namespace {

// The text dump writes an absent object reference as this literal, and a present one as its
// quoted name. Both helpers below are inlined at every use in the image, so this translation unit
// has its own copies rather than sharing the ones mesh.cpp uses.
constexpr char kNoObject[] = "no object";

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

template <class T>
void ReadObjectRef(Stream &stream, T *&refOut) {
    HxStr name(nullptr);
    stream.ReadString(name);
    refOut = dynamic_cast<T *>(g_manager.Find(name));
}

void PrintVectorHeader(FailSink &sink, unsigned nCount) {
    sink.Print("(size:");
    sink.Format("%u", nCount);
    sink.Print(")");
}

void PrintElementIndex(FailSink &sink, unsigned nIndex) {
    sink.Print("\n");
    sink.Format("%d", nIndex);
    sink.Print("\t");
}

// Every labelled number in the dumps is a Print of the label and then a Format of the value alone.
// The FailSink format buffer therefore ends with the bare number.
void PrintFloatField(FailSink &sink, const char *pszLabel, float flValue) {
    sink.Print(pszLabel);
    sink.Format("%.2f", flValue);
}

// NTSC-U/C: 0x004906a8, PAL: 0x004ce530
FailSink &DumpPointsVector(FailSink &sink, const std::vector<Vector3> &values) {
    PrintVectorHeader(sink, values.size());
    for (unsigned nIndex = 0; nIndex < values.size(); ++nIndex) {
        PrintElementIndex(sink, nIndex);
        PrintFloatField(sink, "(x:", values[nIndex].x);
        PrintFloatField(sink, " y:", values[nIndex].y);
        PrintFloatField(sink, " z:", values[nIndex].z);
        sink.Print(")");
    }
    return sink;
}

// NTSC-U/C: 0x004909c0, PAL: 0x004ce848
FailSink &DumpTexsVector(FailSink &sink, const std::vector<Vector2> &values) {
    PrintVectorHeader(sink, values.size());
    for (unsigned nIndex = 0; nIndex < values.size(); ++nIndex) {
        PrintElementIndex(sink, nIndex);
        PrintFloatField(sink, "(x:", values[nIndex].x);
        PrintFloatField(sink, " y:", values[nIndex].y);
        sink.Print(")");
    }
    return sink;
}

// NTSC-U/C: 0x00490cb0, PAL: 0x004ceb38
FailSink &DumpColorsVector(FailSink &sink, const std::vector<Color> &values) {
    PrintVectorHeader(sink, values.size());
    for (unsigned nIndex = 0; nIndex < values.size(); ++nIndex) {
        PrintElementIndex(sink, nIndex);
        PrintFloatField(sink, "(r:", values[nIndex].r);
        PrintFloatField(sink, " g:", values[nIndex].g);
        PrintFloatField(sink, " b:", values[nIndex].b);
        PrintFloatField(sink, " a:", values[nIndex].a);
        sink.Print(")");
    }
    return sink;
}

// A channel dump opens with the keyframe count and then writes one line per keyframe, its index,
// its frame, and the whole value vector. The count comes from std::list::size(), which is a walk
// on this library.
// NTSC-U/C: 0x00490840, PAL: 0x004ce6c8
FailSink &DumpPointsKeys(FailSink &sink, const std::list<MeshAnim::PointsKey> &keys) {
    PrintVectorHeader(sink, keys.size());
    unsigned nIndex = 0;
    for (const auto &key : keys) {
        PrintElementIndex(sink, nIndex);
        ++nIndex;
        sink.Print("(frame:");
        sink.Format("%.2f", key.mFrame);
        sink.Print(" value:");
        DumpPointsVector(sink, key.mValues);
        sink.Print(")");
    }
    return sink;
}

// NTSC-U/C: 0x00490b30, PAL: 0x004ce9b8
FailSink &DumpTexsKeys(FailSink &sink, const std::list<MeshAnim::TexsKey> &keys) {
    PrintVectorHeader(sink, keys.size());
    unsigned nIndex = 0;
    for (const auto &key : keys) {
        PrintElementIndex(sink, nIndex);
        ++nIndex;
        sink.Print("(frame:");
        sink.Format("%.2f", key.mFrame);
        sink.Print(" value:");
        DumpTexsVector(sink, key.mValues);
        sink.Print(")");
    }
    return sink;
}

// NTSC-U/C: 0x00490e78, PAL: 0x004ced00
FailSink &DumpColorsKeys(FailSink &sink, const std::list<MeshAnim::ColorsKey> &keys) {
    PrintVectorHeader(sink, keys.size());
    unsigned nIndex = 0;
    for (const auto &key : keys) {
        PrintElementIndex(sink, nIndex);
        ++nIndex;
        sink.Print("(frame:");
        sink.Format("%.2f", key.mFrame);
        sink.Print(" value:");
        DumpColorsVector(sink, key.mValues);
        sink.Print(")");
    }
    return sink;
}

// The three value vectors serialise as a count and then the components of every element, so the
// padding word of a Vector3 never arrives at a file.
// NTSC-U/C: 0x00490ff8, PAL: 0x004cee80
Stream &WritePointsVector(Stream &stream, const std::vector<Vector3> &values) {
    const int nCount = static_cast<int>(values.size());
    stream.Write(&nCount, sizeof(nCount));
    for (const auto &value : values) {
        stream.Write(&value.x, sizeof(float));
        stream.Write(&value.y, sizeof(float));
        stream.Write(&value.z, sizeof(float));
    }
    return stream;
}

// NTSC-U/C: 0x004911d8, PAL: 0x004cf060
// Unlike the other two channels, a texture-coordinate key writes its frame here rather than in
// WriteTexsKeys().
Stream &WriteTexsKey(Stream &stream, const MeshAnim::TexsKey &key) {
    const int nCount = static_cast<int>(key.mValues.size());
    stream.Write(&nCount, sizeof(nCount));
    for (const auto &value : key.mValues) {
        stream.Write(&value.x, sizeof(float));
        stream.Write(&value.y, sizeof(float));
    }
    stream.Write(&key.mFrame, sizeof(key.mFrame));
    return stream;
}

// NTSC-U/C: 0x00491390, PAL: 0x004cf218
Stream &WriteColorsVector(Stream &stream, const std::vector<Color> &values) {
    const int nCount = static_cast<int>(values.size());
    stream.Write(&nCount, sizeof(nCount));
    for (const auto &value : values) {
        stream.Write(&value.r, sizeof(float));
        stream.Write(&value.g, sizeof(float));
        stream.Write(&value.b, sizeof(float));
        stream.Write(&value.a, sizeof(float));
    }
    return stream;
}

// NTSC-U/C: 0x004910f0, PAL: 0x004cef78
Stream &WritePointsKeys(Stream &stream, const std::list<MeshAnim::PointsKey> &keys) {
    const int nCount = static_cast<int>(keys.size());
    stream.Write(&nCount, sizeof(nCount));
    for (const auto &key : keys) {
        WritePointsVector(stream, key.mValues);
        stream.Write(&key.mFrame, sizeof(key.mFrame));
    }
    return stream;
}

// NTSC-U/C: 0x004912d8, PAL: 0x004cf160
Stream &WriteTexsKeys(Stream &stream, const std::list<MeshAnim::TexsKey> &keys) {
    const int nCount = static_cast<int>(keys.size());
    stream.Write(&nCount, sizeof(nCount));
    for (const auto &key : keys) {
        WriteTexsKey(stream, key);
    }
    return stream;
}

// NTSC-U/C: 0x004914a8, PAL: 0x004cf330
Stream &WriteColorsKeys(Stream &stream, const std::list<MeshAnim::ColorsKey> &keys) {
    const int nCount = static_cast<int>(keys.size());
    stream.Write(&nCount, sizeof(nCount));
    for (const auto &key : keys) {
        WriteColorsVector(stream, key.mValues);
        stream.Write(&key.mFrame, sizeof(key.mFrame));
    }
    return stream;
}

// NTSC-U/C: 0x00491678, PAL: 0x004cf500
Stream &ReadPointsVector(Stream &stream, std::vector<Vector3> &values) {
    int nCount = 0;
    stream.Read(&nCount, sizeof(nCount));
    values.resize(nCount);
    for (auto &value : values) {
        stream.Read(&value.x, sizeof(float));
        stream.Read(&value.y, sizeof(float));
        stream.Read(&value.z, sizeof(float));
    }
    return stream;
}

// NTSC-U/C: 0x00491b10, PAL: 0x004cf998
Stream &ReadTexsVector(Stream &stream, std::vector<Vector2> &values) {
    int nCount = 0;
    stream.Read(&nCount, sizeof(nCount));
    values.resize(nCount);
    for (auto &value : values) {
        stream.Read(&value.x, sizeof(float));
        stream.Read(&value.y, sizeof(float));
    }
    return stream;
}

// NTSC-U/C: 0x00491f80, PAL: 0x004cfe08
Stream &ReadColorsVector(Stream &stream, std::vector<Color> &values) {
    int nCount = 0;
    stream.Read(&nCount, sizeof(nCount));
    values.resize(nCount);
    for (auto &value : values) {
        stream.Read(&value.r, sizeof(float));
        stream.Read(&value.g, sizeof(float));
        stream.Read(&value.b, sizeof(float));
        stream.Read(&value.a, sizeof(float));
    }
    return stream;
}

// NTSC-U/C: 0x004917b0, PAL: 0x004cf638
Stream &ReadPointsKeys(Stream &stream, std::list<MeshAnim::PointsKey> &keys) {
    int nCount = 0;
    stream.Read(&nCount, sizeof(nCount));
    keys.resize(nCount);
    for (auto &key : keys) {
        ReadPointsVector(stream, key.mValues);
        stream.Read(&key.mFrame, sizeof(key.mFrame));
    }
    return stream;
}

// NTSC-U/C: 0x00491c20, PAL: 0x004cfaa8
Stream &ReadTexsKeys(Stream &stream, std::list<MeshAnim::TexsKey> &keys) {
    int nCount = 0;
    stream.Read(&nCount, sizeof(nCount));
    keys.resize(nCount);
    for (auto &key : keys) {
        ReadTexsVector(stream, key.mValues);
        stream.Read(&key.mFrame, sizeof(key.mFrame));
    }
    return stream;
}

// NTSC-U/C: 0x004920c8, PAL: 0x004cff50
Stream &ReadColorsKeys(Stream &stream, std::list<MeshAnim::ColorsKey> &keys) {
    int nCount = 0;
    stream.Read(&nCount, sizeof(nCount));
    keys.resize(nCount);
    for (auto &key : keys) {
        ReadColorsVector(stream, key.mValues);
        stream.Read(&key.mFrame, sizeof(key.mFrame));
    }
    return stream;
}

// The frame of the last keyframe of a channel, and zero for an empty channel. The image counts the
// list rather than testing its sentinel, so the emptiness test is a size comparison.
template <class Key>
float ChannelEndFrame(const std::list<Key> &keys) {
    if (keys.size() == 0) {
        return 0.0f;
    }
    return keys.back().mFrame;
}

// Select the keyframe pair bracketing flFrame and the blend between them. A frame at or before the
// first key yields that key with a blend of zero, and a frame at or after the last key yields that
// key with a blend of one, so the channel clamps rather than extrapolating. The image inlines this
// three times, once per channel.
template <class Key>
void SelectKeyPair(
    const std::list<Key> &keys, float flFrame, const Key *&pFrom, const Key *&pTo, float &flBlend) {
    if (flFrame <= keys.front().mFrame) {
        pFrom = &keys.front();
        pTo = &keys.front();
        flBlend = 0.0f;
        return;
    }
    if (keys.back().mFrame <= flFrame) {
        pFrom = &keys.back();
        pTo = &keys.back();
        flBlend = 1.0f;
        return;
    }

    auto itFrom = keys.begin();
    auto itTo = itFrom;
    ++itTo;
    while (itTo != keys.end() && flFrame > itTo->mFrame) {
        itFrom = itTo;
        ++itTo;
    }
    pFrom = &*itFrom;
    pTo = &*itTo;
    flBlend = (flFrame - itFrom->mFrame) / (itTo->mFrame - itFrom->mFrame);
}

// A channel may store more values than the mesh has vertices, and the surplus is discarded.
inline unsigned BlendCount(unsigned nValueCount, unsigned nVertCount) {
    return nValueCount < nVertCount ? nValueCount : nVertCount;
}

// NTSC-U/C: 0x00487998, PAL: 0x004c57b8
void BlendPointsIntoVerts(const std::vector<Vector3> &from,
                          const std::vector<Vector3> &to,
                          std::vector<MeshVert> &verts,
                          float flBlend) {
    const unsigned nCount = BlendCount(from.size(), verts.size());
    if (flBlend == 0.0f) {
        for (unsigned nIndex = 0; nIndex < nCount; ++nIndex) {
            verts[nIndex].mPoint = from[nIndex];
        }
        return;
    }
    if (flBlend == 1.0f) {
        for (unsigned nIndex = 0; nIndex < nCount; ++nIndex) {
            verts[nIndex].mPoint = to[nIndex];
        }
        return;
    }
    // The blend runs on VU0 as a broadcast multiply-accumulate over the xyz field, so the padding
    // word of the destination is not written on this path while the two copies above do write it.
    const float flInverse = 1.0f - flBlend;
    for (unsigned nIndex = 0; nIndex < nCount; ++nIndex) {
        verts[nIndex].mPoint.x = to[nIndex].x * flBlend + from[nIndex].x * flInverse;
        verts[nIndex].mPoint.y = to[nIndex].y * flBlend + from[nIndex].y * flInverse;
        verts[nIndex].mPoint.z = to[nIndex].z * flBlend + from[nIndex].z * flInverse;
    }
}

// NTSC-U/C: 0x00487aa8, PAL: 0x004c58c8
void BlendTexsIntoVerts(const std::vector<Vector2> &from,
                        const std::vector<Vector2> &to,
                        std::vector<MeshVert> &verts,
                        float flBlend) {
    const unsigned nCount = BlendCount(from.size(), verts.size());
    if (flBlend == 0.0f) {
        for (unsigned nIndex = 0; nIndex < nCount; ++nIndex) {
            verts[nIndex].mTex1 = from[nIndex];
        }
        return;
    }
    if (flBlend == 1.0f) {
        for (unsigned nIndex = 0; nIndex < nCount; ++nIndex) {
            verts[nIndex].mTex1 = to[nIndex];
        }
        return;
    }
    for (unsigned nIndex = 0; nIndex < nCount; ++nIndex) {
        verts[nIndex].mTex1.x = (to[nIndex].x - from[nIndex].x) * flBlend + from[nIndex].x;
        verts[nIndex].mTex1.y = (to[nIndex].y - from[nIndex].y) * flBlend + from[nIndex].y;
    }
}

// NTSC-U/C: 0x00487bc8, PAL: 0x004c59e8
void BlendColorsIntoVerts(const std::vector<Color> &from,
                          const std::vector<Color> &to,
                          std::vector<MeshVert> &verts,
                          float flBlend) {
    const unsigned nCount = BlendCount(from.size(), verts.size());
    if (flBlend == 0.0f) {
        for (unsigned nIndex = 0; nIndex < nCount; ++nIndex) {
            verts[nIndex].mColor = from[nIndex];
        }
        return;
    }
    if (flBlend == 1.0f) {
        for (unsigned nIndex = 0; nIndex < nCount; ++nIndex) {
            verts[nIndex].mColor = to[nIndex];
        }
        return;
    }
    const float flInverse = 1.0f - flBlend;
    for (unsigned nIndex = 0; nIndex < nCount; ++nIndex) {
        verts[nIndex].mColor.r = to[nIndex].r * flBlend + from[nIndex].r * flInverse;
        verts[nIndex].mColor.g = to[nIndex].g * flBlend + from[nIndex].g * flInverse;
        verts[nIndex].mColor.b = to[nIndex].b * flBlend + from[nIndex].b * flInverse;
        verts[nIndex].mColor.a = to[nIndex].a * flBlend + from[nIndex].a * flInverse;
    }
}

} // namespace

// NTSC-U/C: 0x006eed70, PAL: 0x00732790
HxStr g_meshAnimClassName("MeshAnim");

// NTSC-U/C: 0x00493a00, PAL: 0x004d18b0
Object *CreateRegisteredMeshAnim(const HxStr &name) {
    // The binary rounds the 0x48-byte object up to the allocator's own granularity.
    return new MeshAnim(name);
}

// NTSC-U/C: 0x00493170, PAL: 0x004d1020
MeshAnim *NewMeshAnim(const HxStr &name) {
    return new MeshAnim(name);
}

// NTSC-U/C: 0x00493520, PAL: 0x004d13d0
MeshAnim::MeshAnim(const HxStr &name) : Object(name), mMesh(nullptr), mKeysOwner(this) {
}

// NTSC-U/C: 0x00493240, PAL: 0x004d10f0
MeshAnim::~MeshAnim() {
    RemoveObjectRefs();
    ReleaseAllRefs();
}

// NTSC-U/C: 0x004873b0, PAL: 0x004c51d0
float MeshAnim::EndFrame() {
    const float flPoints = ChannelEndFrame(mKeysOwner->mVertPointsKeys);
    const float flTexs = ChannelEndFrame(mKeysOwner->mVertTexsKeys);
    const float flColors = ChannelEndFrame(mKeysOwner->mVertColorsKeys);
    return std::max(flPoints, std::max(flTexs, flColors));
}

// NTSC-U/C: 0x00486c98, PAL: 0x004c4a68
void MeshAnim::DumpText(FailSink &sink) {
    Object::DumpText(sink);
    Animatable::DumpText(sink);
    if (sink.mDumpLevel <= 0) {
        return;
    }

    sink.Print("[MeshAnim]\n");
    sink.Print("light:");
    PrintObjectRef(sink, mMesh);
    sink.Print(" keysOwner:");
    PrintObjectRef(sink, mKeysOwner);
    sink.Print("\n");

    sink.Print("vertPointsKeys:");
    DumpPointsKeys(sink, mVertPointsKeys);
    sink.Print("\n");
    sink.Print("vertTexsKeys:");
    DumpTexsKeys(sink, mVertTexsKeys);
    sink.Print("\n");
    sink.Print("vertColorsKeys:");
    DumpColorsKeys(sink, mVertColorsKeys);
    sink.Print("\n");
}

// NTSC-U/C: 0x00486e50, PAL: 0x004c4c20
void MeshAnim::Save(Stream &stream) {
    const int nVersion = kSerialVersion;
    stream.Write(&nVersion, sizeof(nVersion));

    Animatable::Save(stream);

    WriteObjectRef(stream, mMesh);
    WritePointsKeys(stream, mVertPointsKeys);
    WriteTexsKeys(stream, mVertTexsKeys);
    WriteColorsKeys(stream, mVertColorsKeys);
    WriteObjectRef(stream, mKeysOwner);
}

// NTSC-U/C: 0x00486ac0, PAL: 0x004c4890
void MeshAnim::Replace(Object *pFrom, Object *pTo) {
    Animatable::Replace(pFrom, pTo);

    if (mMesh == pFrom && mMesh != nullptr) {
        pFrom->RemoveRef(this);
        mMesh = dynamic_cast<Mesh *>(pTo);
        if (mMesh != nullptr) {
            mMesh->AddRef(this);
        }
    }

    if (pTo != nullptr) {
        if (mKeysOwner == pFrom && mKeysOwner != nullptr) {
            pFrom->RemoveRef(this);
            mKeysOwner = dynamic_cast<MeshAnim *>(pTo);
            if (mKeysOwner != nullptr) {
                mKeysOwner->AddRef(this);
            }
        }
        return;
    }

    // Losing the animation that owned the shared keys takes a copy of the channels rather than
    // discarding them. No reference is dropped on this path.
    if (mKeysOwner == pFrom) { // Yes, a null owner matching a null pFrom is read through.
        mVertPointsKeys = mKeysOwner->mVertPointsKeys;
        mVertTexsKeys = mKeysOwner->mVertTexsKeys;
        mVertColorsKeys = mKeysOwner->mVertColorsKeys;
        mKeysOwner = this;
    }
}

// NTSC-U/C: 0x00493510, PAL: 0x004d13c0
const HxStr &MeshAnim::ClassName() const {
    return g_meshAnimClassName;
}

// NTSC-U/C: 0x00487258, PAL: 0x004c5078
void MeshAnim::Copy(const Object *pSource, unsigned nFlags) {
    const MeshAnim *pSourceAnim = dynamic_cast<const MeshAnim *>(pSource);

    Animatable::Copy(pSource, nFlags);
    RemoveObjectRefs();

    mMesh = pSourceAnim->mMesh;
    if ((nFlags & kCopyShareKeys) != 0 || pSourceAnim->mKeysOwner != pSourceAnim) {
        mKeysOwner = pSourceAnim->mKeysOwner;
        ClearKeys();
    } else {
        mKeysOwner = this;
        mVertPointsKeys = pSourceAnim->mVertPointsKeys;
        mVertTexsKeys = pSourceAnim->mVertTexsKeys;
        mVertColorsKeys = pSourceAnim->mVertColorsKeys;
    }

    AddObjectRefs();
}

// NTSC-U/C: 0x00486fa8, PAL: 0x004c4d78
void MeshAnim::Load(Stream &stream) {
    int nVersion = 0;
    stream.Read(&nVersion, sizeof(nVersion));
    if (nVersion > kSerialVersion) {
        g_failSink.Report("Can't load new MeshAnim\n");
        return;
    }

    Animatable::Load(stream);
    RemoveObjectRefs();

    ReadObjectRef(stream, mMesh);
    ReadPointsKeys(stream, mVertPointsKeys);
    ReadTexsKeys(stream, mVertTexsKeys);
    ReadColorsKeys(stream, mVertColorsKeys);
    ReadObjectRef(stream, mKeysOwner);

    // With kSerialVersion at zero the version test can never fail here, because a higher version
    // has already returned above.
    if (nVersion <= kSerialVersion && mKeysOwner != this) {
        mVertPointsKeys.clear();
        mVertTexsKeys.clear();
        mVertColorsKeys.clear();
    }

    AddObjectRefs();
}

// NTSC-U/C: 0x004867d8, PAL: 0x004c45a8
void MeshAnim::CopyVertKeys(int nFromVert, int nToVert) {
    for (auto &key : mKeysOwner->mVertPointsKeys) {
        key.mValues[nToVert] = key.mValues[nFromVert];
    }
    for (auto &key : mKeysOwner->mVertTexsKeys) {
        key.mValues[nToVert] = key.mValues[nFromVert];
    }
    for (auto &key : mKeysOwner->mVertColorsKeys) {
        key.mValues[nToVert] = key.mValues[nFromVert];
    }
}

// NTSC-U/C: 0x00486900, PAL: 0x004c46d0
void MeshAnim::AppendVertKeys(int nVert) {
    for (auto &key : mKeysOwner->mVertPointsKeys) {
        key.mValues.push_back(key.mValues[nVert]);
    }
    for (auto &key : mKeysOwner->mVertTexsKeys) {
        key.mValues.push_back(key.mValues[nVert]);
    }
    for (auto &key : mKeysOwner->mVertColorsKeys) {
        key.mValues.push_back(key.mValues[nVert]);
    }
}

// NTSC-U/C: 0x00487518, PAL: 0x004c5338
void MeshAnim::SetFrameSelf(float flFrame) {
    if (mMesh == nullptr) {
        return;
    }

    if (mKeysOwner->mVertPointsKeys.size() != 0) {
        const PointsKey *pFrom = nullptr;
        const PointsKey *pTo = nullptr;
        float flBlend = 0.0f;
        SelectKeyPair(mKeysOwner->mVertPointsKeys, flFrame, pFrom, pTo, flBlend);
        BlendPointsIntoVerts(pFrom->mValues, pTo->mValues, mMesh->mVertsOwner->mVerts, flBlend);
        mMesh->SyncChanged(Mesh::kSyncPoints);
    }

    if (mKeysOwner->mVertTexsKeys.size() != 0) {
        const TexsKey *pFrom = nullptr;
        const TexsKey *pTo = nullptr;
        float flBlend = 0.0f;
        SelectKeyPair(mKeysOwner->mVertTexsKeys, flFrame, pFrom, pTo, flBlend);
        BlendTexsIntoVerts(pFrom->mValues, pTo->mValues, mMesh->mVertsOwner->mVerts, flBlend);
        mMesh->SyncChanged(Mesh::kSyncTexs);
    }

    if (mKeysOwner->mVertColorsKeys.size() != 0) {
        const ColorsKey *pFrom = nullptr;
        const ColorsKey *pTo = nullptr;
        float flBlend = 0.0f;
        SelectKeyPair(mKeysOwner->mVertColorsKeys, flFrame, pFrom, pTo, flBlend);
        BlendColorsIntoVerts(pFrom->mValues, pTo->mValues, mMesh->mVertsOwner->mVerts, flBlend);
        mMesh->SyncChanged(Mesh::kSyncColors);
    }
}

// NTSC-U/C: 0x00494120, PAL: 0x004d1fd0
void MeshAnim::SetMesh(Mesh *pMesh) {
    if (mMesh != nullptr) {
        mMesh->RemoveRef(this);
    }
    mMesh = pMesh;
    if (pMesh != nullptr) {
        pMesh->AddRef(this);
    }
}

// NTSC-U/C: 0x00494288, PAL: 0x004d2138
void MeshAnim::AddObjectRefs() {
    if (mMesh != nullptr) {
        mMesh->AddRef(this);
    }
    if (mKeysOwner != nullptr) {
        mKeysOwner->AddRef(this);
    }
}

// NTSC-U/C: 0x00494238, PAL: 0x004d20e8
void MeshAnim::RemoveObjectRefs() {
    if (mMesh != nullptr) {
        mMesh->RemoveRef(this);
    }
    if (mKeysOwner != nullptr) {
        mKeysOwner->RemoveRef(this);
    }
}

// NTSC-U/C: 0x00494178, PAL: 0x004d2028
void MeshAnim::ClearKeys() {
    if (mKeysOwner == this) {
        return;
    }
    mVertPointsKeys.clear();
    mVertTexsKeys.clear();
    mVertColorsKeys.clear();
}

// NTSC-U/C: 0x004941c0, PAL: 0x004d2070
void MeshAnim::SetKeysOwner(MeshAnim *pOwner) {
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
