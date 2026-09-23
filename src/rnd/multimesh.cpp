#include "rnd/multimesh.h"

#include <list>

#include "math/transform.h"
#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mesh.h"
#include "rnd/stream.h"
#include "rnd/transformable.h"

namespace Rnd {

namespace {

// The text dump writes an absent object reference as this literal, and a present one as its
// quoted name. Both helpers are inlined at every use in the image, so this translation unit has
// its own copies.
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

void PrintRow(FailSink &sink, const Vector3 &row) {
    sink.Print("\n\t");
    sink.Print("(x:");
    sink.Format("%.2f", row.x);
    sink.Print(" y:");
    sink.Format("%.2f", row.y);
    sink.Print(" z:");
    sink.Format("%.2f", row.z);
    sink.Print(")");
}

void PrintTransform(FailSink &sink, const Transform &xfm) {
    PrintRow(sink, xfm.mBasisX);
    PrintRow(sink, xfm.mBasisY);
    PrintRow(sink, xfm.mBasisZ);
    PrintRow(sink, xfm.mTranslation);
}

// 0x004eae58
FailSink &DumpTransformList(FailSink &sink, const std::list<Transform> &transforms) {
    sink.Print("(size:");
    sink.Format("%u", transforms.size());
    sink.Print(")");

    unsigned nIndex = 0;
    for (const auto &xfm : transforms) {
        sink.Print("\n");
        sink.Format("%d", nIndex);
        ++nIndex;
        sink.Print("\t");
        PrintTransform(sink, xfm);
    }
    return sink;
}

// A transform row writes only its first three floats, so the padding word never arrives at a file.
void WriteRow(Stream &stream, const Vector3 &row) {
    stream.Write(&row.x, sizeof(float));
    stream.Write(&row.y, sizeof(float));
    stream.Write(&row.z, sizeof(float));
}

// 0x004eb240
Stream &WriteTransformList(Stream &stream, const std::list<Transform> &transforms) {
    const int nCount = static_cast<int>(transforms.size());
    stream.Write(&nCount, sizeof(nCount));
    for (const auto &xfm : transforms) {
        WriteRow(stream, xfm.mBasisX);
        WriteRow(stream, xfm.mBasisY);
        WriteRow(stream, xfm.mBasisZ);
        WriteRow(stream, xfm.mTranslation);
    }
    return stream;
}

void ReadRow(Stream &stream, Vector3 &row) {
    stream.Read(&row.x, sizeof(float));
    stream.Read(&row.y, sizeof(float));
    stream.Read(&row.z, sizeof(float));
}

// 0x004eb6a0. Ghidra titles this routine WriteInstances, which is wrong. Load() is its only caller
// and every transfer goes through the read slot of the stream.
Stream &ReadTransformList(Stream &stream, std::list<Transform> &transforms) {
    int nCount = 0;
    stream.Read(&nCount, sizeof(nCount));

    // Every new element starts from a prototype whose four padding words are 1.0, which the reader
    // then does not overwrite.
    Transform prototype;
    prototype.mBasisX.w = 1.0f;
    prototype.mBasisY.w = 1.0f;
    prototype.mBasisZ.w = 1.0f;
    prototype.mTranslation.w = 1.0f;
    transforms.resize(nCount, prototype);

    for (auto &xfm : transforms) {
        ReadRow(stream, xfm.mBasisX);
        ReadRow(stream, xfm.mBasisY);
        ReadRow(stream, xfm.mBasisZ);
        ReadRow(stream, xfm.mTranslation);
    }
    return stream;
}

// Rnd::Transformable stores its two transforms as float rows rather than as a Transform, so the
// copies below spell out the conversion. The image moves each row as one 128-bit quadword.
void CopyXfm(const float aflSrc[kXfmRowCount][kXfmRowFloatCount],
             float aflDest[kXfmRowCount][kXfmRowFloatCount]) {
    for (int nRow = 0; nRow < kXfmRowCount; ++nRow) {
        for (int nFloat = 0; nFloat < kXfmRowFloatCount; ++nFloat) {
            aflDest[nRow][nFloat] = aflSrc[nRow][nFloat];
        }
    }
}

void CopyRowToXfm(const Vector3 &row, float aflRow[kXfmRowFloatCount]) {
    aflRow[0] = row.x;
    aflRow[1] = row.y;
    aflRow[2] = row.z;
    aflRow[3] = row.w;
}

void CopyTransformToXfm(const Transform &xfm, float aflDest[kXfmRowCount][kXfmRowFloatCount]) {
    CopyRowToXfm(xfm.mBasisX, aflDest[0]);
    CopyRowToXfm(xfm.mBasisY, aflDest[1]);
    CopyRowToXfm(xfm.mBasisZ, aflDest[2]);
    CopyRowToXfm(xfm.mTranslation, aflDest[3]);
}

} // namespace

// 0x004ebbe8
MultiMesh *NewMultiMesh(const HxStr &name) {
    // The binary allocates the 0x38 bytes the object occupies.
    return new MultiMesh(name);
}

// 0x00704070
MultiMesh *(*g_pfnNewMultiMesh)(const HxStr &name) = NewMultiMesh;

// 0x00704068
HxStr g_multiMeshClassName("MultiMesh");

// 0x00895038
int g_nRndMultiMeshLoadVersion;

// 0x004ebb58
Object *CreateRegisteredMultiMesh(const HxStr &name) {
    return g_pfnNewMultiMesh(name);
}

// 0x004e8830
MultiMesh::MultiMesh(const HxStr &name) : Object(name), mMesh(nullptr) {
    // The constructor acquires the mesh reference even though the mesh it has just set is null.
    AcquireMeshRef();
}

// 0x004e85c0
MultiMesh::~MultiMesh() {
    ReleaseMeshRef();
    ReleaseAllRefs();
}

// 0x004e81a0
void MultiMesh::DumpText(FailSink &sink) {
    Object::DumpText(sink);
    Drawable::DumpText(sink);
    if (sink.mDumpLevel <= 0) {
        return;
    }

    sink.Print("[MultiMesh]\n");
    sink.Print("mesh: ");
    PrintObjectRef(sink, mMesh);
    sink.Print(" transforms: ");
    DumpTransformList(sink, mTransforms);
    sink.Print("\n");
}

// 0x004ebd10
void MultiMesh::Save(Stream &stream) {
    const int nVersion = kSerialVersion;
    stream.Write(&nVersion, sizeof(nVersion));

    Drawable::Save(stream);
    WriteObjectRef(stream, mMesh);
    WriteTransformList(stream, mTransforms);
}

// 0x004ebe40
void MultiMesh::Replace(Object *pFrom, Object *pTo) {
    Drawable::Replace(pFrom, pTo);

    if (mMesh == pFrom && mMesh != nullptr) {
        pFrom->RemoveRef(this);
        mMesh = dynamic_cast<Mesh *>(pTo);
        if (mMesh != nullptr) {
            mMesh->AddRef(this);
        }
    }
}

// 0x004eba80
const HxStr &MultiMesh::ClassName() const {
    return g_multiMeshClassName;
}

// 0x004ebc60
void MultiMesh::Copy(const Object *pSource, unsigned nFlags) {
    const MultiMesh *pSourceMulti = dynamic_cast<const MultiMesh *>(pSource);

    Drawable::Copy(pSource, nFlags);
    ReleaseMeshRef();

    mMesh = pSourceMulti->mMesh;
    mTransforms = pSourceMulti->mTransforms;

    AcquireMeshRef();
}

// 0x004e8288
void MultiMesh::Load(Stream &stream) {
    stream.Read(&g_nRndMultiMeshLoadVersion, sizeof(g_nRndMultiMeshLoadVersion));
    if (g_nRndMultiMeshLoadVersion > kSerialVersion) {
        g_failSink.Report("Can't load new MultiMesh\n");
        return;
    }

    Drawable::Load(stream);
    ReleaseMeshRef();

    HxStr name(nullptr);
    stream.ReadString(name);
    mMesh = dynamic_cast<Mesh *>(g_manager.Find(name));

    ReadTransformList(stream, mTransforms);
    AcquireMeshRef();
}

// 0x004ebde0
void MultiMesh::AcquireMeshRef() {
    if (mMesh != nullptr) {
        mMesh->AddRef(this);
    }
}

// 0x004ebe10
void MultiMesh::ReleaseMeshRef() {
    if (mMesh != nullptr) {
        mMesh->RemoveRef(this);
    }
}

// 0x004e83d0
int MultiMesh::DrawSelf() {
    if (mMesh == nullptr) {
        return 1;
    }

    const float flSavedMinScreen = mMesh->mMinScreen;
    float aflSavedLocal[kXfmRowCount][kXfmRowFloatCount];
    float aflSavedWorld[kXfmRowCount][kXfmRowFloatCount];
    CopyXfm(mMesh->mLocalXfm, aflSavedLocal);
    CopyXfm(mMesh->mWorldXfm, aflSavedWorld);

    // The level of detail substitution would replace the mesh part way through the run. It is
    // disabled for the duration.
    mMesh->mMinScreen = 0.0f;
    // Yes, the binary re-points mNext at its current value, which drops and retakes the same
    // reference and changes nothing.
    mMesh->SetNext(mMesh->mNext);

    for (const auto &xfm : mTransforms) {
        CopyTransformToXfm(xfm, mMesh->mLocalXfm);
        mMesh->mDirty = 1;
        mMesh->UpdateWorldXfm(nullptr, 0);
        mMesh->Draw();
    }

    mMesh->mMinScreen = flSavedMinScreen;
    mMesh->SetNext(mMesh->mNext);

    // The saved world transform goes back through the local slot and one recomposition, which
    // restores the world transform the mesh had before the run. The true local transform follows,
    // marked dirty so that the next recomposition uses it.
    CopyXfm(aflSavedWorld, mMesh->mLocalXfm);
    mMesh->mDirty = 1;
    mMesh->UpdateWorldXfm(nullptr, 0);
    CopyXfm(aflSavedLocal, mMesh->mLocalXfm);
    mMesh->mDirty = 1;
    return 1;
}

} // namespace Rnd
