#include "rnd/mesh.h"

#include <vector>

#include "math/vector3.h"
#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/raytest.h"
#include "rnd/stream.h"
#include "rnd/transformable.h"

namespace Rnd {

namespace {

// The text dump writes an absent object reference as this literal, and a present one as its
// quoted name.
constexpr char kNoObject[] = "no object";

// 0x00493f00
FailSink &PrintZMode(FailSink &sink, Mesh::ZMode nZMode) {
    switch (nZMode) {
    case Mesh::kZModeDisable:
        sink.Print("Disable");
        break;
    case Mesh::kZModeZReadOnly:
        sink.Print("ZReadOnly");
        break;
    case Mesh::kZModeZReadWrite:
        sink.Print("ZReadWrite");
        break;
    case Mesh::kZModeWReadOnly:
        sink.Print("WReadOnly");
        break;
    case Mesh::kZModeWReadWrite:
        sink.Print("WReadWrite");
        break;
    }
    return sink;
}

// 0x00482f18
FailSink &PrintZFunc(FailSink &sink, Mesh::ZFunc nZFunc) {
    switch (nZFunc) {
    case Mesh::kZFuncNever:
        sink.Print("Never");
        break;
    case Mesh::kZFuncLess:
        sink.Print("Less");
        break;
    case Mesh::kZFuncEqual:
        sink.Print("Equal");
        break;
    case Mesh::kZFuncLessEqual:
        sink.Print("LessEqual");
        break;
    case Mesh::kZFuncGreater:
        sink.Print("Greater");
        break;
    case Mesh::kZFuncNotEqual:
        sink.Print("NotEqual");
        break;
    case Mesh::kZFuncGreaterEqual:
        sink.Print("GreaterEqual");
        break;
    case Mesh::kZFuncAlways:
        sink.Print("Always");
        break;
    }
    return sink;
}

// The name of an object with no name of its own reads as the empty string. The binary passes a
// global that stores it.
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

// An object reference is stored as a name and resolves through the renderer registry. A name no
// loaded object matches, and the empty name an absent reference writes, both produce null.
template <class T>
void ReadObjectRef(Stream &stream, T *&refOut) {
    HxStr name(nullptr);
    stream.ReadString(name);
    refOut = dynamic_cast<T *>(g_manager.Find(name));
}

void PrintVector3(FailSink &sink, const Vector3 &v) {
    sink.Format("(x:%.2f", v.x);
    sink.Format(" y:%.2f", v.y);
    sink.Format(" z:%.2f", v.z);
    sink.Print(")");
}

// Each vector dump opens with the element count and then writes the index of every element on its
// own line.
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

// 0x004809f0
FailSink &DumpVert(FailSink &sink, const MeshVert &vert) {
    sink.Print("\n\tp:");
    PrintVector3(sink, vert.mPoint);
    sink.Print("\n\tn:");
    PrintVector3(sink, vert.mNorm);
    sink.Print("\n\tc:");
    sink.Format("(r:%.2f", vert.mColor.r);
    sink.Format(" g:%.2f", vert.mColor.g);
    sink.Format(" b:%.2f", vert.mColor.b);
    sink.Format(" a:%.2f", vert.mColor.a);
    sink.Print(")");
    sink.Print("\n\tt1:");
    sink.Format("(x:%.2f", vert.mTex1.x);
    sink.Format(" y:%.2f", vert.mTex1.y);
    sink.Print(")");
    sink.Print(" t2:");
    sink.Format("(x:%.2f", vert.mTex2.x);
    sink.Format(" y:%.2f", vert.mTex2.y);
    sink.Print(")");
    return sink;
}

// 0x0048a5d8
FailSink &DumpVertVector(FailSink &sink, const std::vector<MeshVert> &verts) {
    PrintVectorHeader(sink, verts.size());
    for (unsigned nIndex = 0; nIndex < verts.size(); ++nIndex) {
        PrintElementIndex(sink, nIndex);
        DumpVert(sink, verts[nIndex]);
    }
    return sink;
}

// 0x0048a6c8
FailSink &DumpFaceVector(FailSink &sink, const std::vector<MeshFace> &faces) {
    PrintVectorHeader(sink, faces.size());
    for (unsigned nIndex = 0; nIndex < faces.size(); ++nIndex) {
        PrintElementIndex(sink, nIndex);
        sink.Format("(v1:%hu", faces[nIndex].mV1);
        sink.Format(" v2:%hu", faces[nIndex].mV2);
        sink.Format(" v3:%hu", faces[nIndex].mV3);
        sink.Print(")");
    }
    return sink;
}

// 0x0048a858
FailSink &DumpEdgeVector(FailSink &sink, const std::vector<MeshEdge> &edges) {
    PrintVectorHeader(sink, edges.size());
    for (unsigned nIndex = 0; nIndex < edges.size(); ++nIndex) {
        PrintElementIndex(sink, nIndex);
        sink.Format("(v1:%hu", edges[nIndex].mV1);
        sink.Format(" v2:%hu", edges[nIndex].mV2);
        sink.Print(")");
    }
    return sink;
}

// 0x0048a9b0
Stream &WriteVertVector(Stream &stream, const std::vector<MeshVert> &verts) {
    const int nCount = static_cast<int>(verts.size());
    stream.Write(&nCount, sizeof(nCount));
    for (const auto &vert : verts) {
        stream.Write(&vert.mPoint.x, sizeof(float));
        stream.Write(&vert.mPoint.y, sizeof(float));
        stream.Write(&vert.mPoint.z, sizeof(float));
        stream.Write(&vert.mNorm.x, sizeof(float));
        stream.Write(&vert.mNorm.y, sizeof(float));
        stream.Write(&vert.mNorm.z, sizeof(float));
        stream.Write(&vert.mColor.r, sizeof(float));
        stream.Write(&vert.mColor.g, sizeof(float));
        stream.Write(&vert.mColor.b, sizeof(float));
        stream.Write(&vert.mColor.a, sizeof(float));
        stream.Write(&vert.mTex1.x, sizeof(float));
        stream.Write(&vert.mTex1.y, sizeof(float));
        stream.Write(&vert.mTex2.x, sizeof(float));
        stream.Write(&vert.mTex2.y, sizeof(float));
    }
    return stream;
}

// 0x0048ac40
Stream &WriteFaceVector(Stream &stream, const std::vector<MeshFace> &faces) {
    const int nCount = static_cast<int>(faces.size());
    stream.Write(&nCount, sizeof(nCount));
    for (const auto &face : faces) {
        stream.Write(&face.mV1, sizeof(face.mV1));
        stream.Write(&face.mV2, sizeof(face.mV2));
        stream.Write(&face.mV3, sizeof(face.mV3));
    }
    return stream;
}

// 0x004945e0
Stream &WriteEdgeVector(Stream &stream, const std::vector<MeshEdge> &edges) {
    const int nCount = static_cast<int>(edges.size());
    stream.Write(&nCount, sizeof(nCount));
    for (const auto &edge : edges) {
        stream.Write(&edge.mV1, sizeof(edge.mV1));
        stream.Write(&edge.mV2, sizeof(edge.mV2));
    }
    return stream;
}

// 0x00482368
Stream &ReadVert(Stream &stream, MeshVert &vert) {
    stream.Read(&vert.mPoint.x, sizeof(float));
    stream.Read(&vert.mPoint.y, sizeof(float));
    stream.Read(&vert.mPoint.z, sizeof(float));
    stream.Read(&vert.mNorm.x, sizeof(float));
    stream.Read(&vert.mNorm.y, sizeof(float));
    stream.Read(&vert.mNorm.z, sizeof(float));
    stream.Read(&vert.mColor.r, sizeof(float));
    stream.Read(&vert.mColor.g, sizeof(float));
    stream.Read(&vert.mColor.b, sizeof(float));
    stream.Read(&vert.mColor.a, sizeof(float));
    stream.Read(&vert.mTex1.x, sizeof(float));
    stream.Read(&vert.mTex1.y, sizeof(float));
    stream.Read(&vert.mTex2.x, sizeof(float));
    stream.Read(&vert.mTex2.y, sizeof(float));
    return stream;
}

// 0x0048ad48
Stream &ReadVertVector(Stream &stream, std::vector<MeshVert> &verts) {
    int nCount = 0;
    stream.Read(&nCount, sizeof(nCount));
    verts.resize(nCount);
    for (auto &vert : verts) {
        ReadVert(stream, vert);
    }
    return stream;
}

// 0x0048ae78
Stream &ReadFaceVector(Stream &stream, std::vector<MeshFace> &faces) {
    int nCount = 0;
    stream.Read(&nCount, sizeof(nCount));
    faces.resize(nCount);
    for (auto &face : faces) {
        stream.Read(&face.mV1, sizeof(face.mV1));
        stream.Read(&face.mV2, sizeof(face.mV2));
        stream.Read(&face.mV3, sizeof(face.mV3));
    }
    return stream;
}

// 0x0048af78
Stream &ReadEdgeVector(Stream &stream, std::vector<MeshEdge> &edges) {
    int nCount = 0;
    stream.Read(&nCount, sizeof(nCount));
    edges.resize(nCount);
    for (auto &edge : edges) {
        stream.Read(&edge.mV1, sizeof(edge.mV1));
        stream.Read(&edge.mV2, sizeof(edge.mV2));
    }
    return stream;
}

// The cross product is a VU0 outer-product pair in the image, vopmula followed by vopmsub, rather
// than a call.
inline void Vec3Cross(const float *pLeft, const float *pRight, float *pOut) {
    pOut[0] = pLeft[1] * pRight[2] - pLeft[2] * pRight[1];
    pOut[1] = pLeft[2] * pRight[0] - pLeft[0] * pRight[2];
    pOut[2] = pLeft[0] * pRight[1] - pLeft[1] * pRight[0];
}

// Also inlined in the image, as the VU0 sequence vmulax, vmadday, vmaddaz, vmaddw.
inline void
TransformPoint(const float aflXfm[kXfmRowCount][kXfmRowFloatCount], const float *pIn, float *pOut) {
    pOut[0] = aflXfm[0][0] * pIn[0] + aflXfm[1][0] * pIn[1] + aflXfm[2][0] * pIn[2] + aflXfm[3][0];
    pOut[1] = aflXfm[0][1] * pIn[0] + aflXfm[1][1] * pIn[1] + aflXfm[2][1] * pIn[2] + aflXfm[3][1];
    pOut[2] = aflXfm[0][2] * pIn[0] + aflXfm[1][2] * pIn[1] + aflXfm[2][2] * pIn[2] + aflXfm[3][2];
}

// De-inlined from the head of Mesh::Collide, which inverts the owner's world transform by hand
// rather than through a helper.
inline void InvertXfm(const float aflWorld[kXfmRowCount][kXfmRowFloatCount],
                      float aflInverse[kXfmRowCount][kXfmRowFloatCount]) {
    const float flDet =
        aflWorld[0][0] * (aflWorld[1][1] * aflWorld[2][2] - aflWorld[2][1] * aflWorld[1][2]) -
        aflWorld[0][1] * (aflWorld[1][0] * aflWorld[2][2] - aflWorld[2][0] * aflWorld[1][2]) +
        aflWorld[0][2] * (aflWorld[1][0] * aflWorld[2][1] - aflWorld[2][0] * aflWorld[1][1]);
    // Yes, a singular transform produces a zero scale rather than a reported failure.
    const float flScale = flDet != 0.0f ? 1.0f / flDet : 0.0f;

    aflInverse[0][0] =
        (aflWorld[1][1] * aflWorld[2][2] - aflWorld[2][1] * aflWorld[1][2]) * flScale;
    aflInverse[0][1] =
        (aflWorld[2][1] * aflWorld[0][2] - aflWorld[0][1] * aflWorld[2][2]) * flScale;
    aflInverse[0][2] =
        (aflWorld[0][1] * aflWorld[1][2] - aflWorld[1][1] * aflWorld[0][2]) * flScale;
    aflInverse[1][0] =
        (aflWorld[2][0] * aflWorld[1][2] - aflWorld[1][0] * aflWorld[2][2]) * flScale;
    aflInverse[1][1] =
        (aflWorld[0][0] * aflWorld[2][2] - aflWorld[2][0] * aflWorld[0][2]) * flScale;
    aflInverse[1][2] =
        (aflWorld[1][0] * aflWorld[0][2] - aflWorld[0][0] * aflWorld[1][2]) * flScale;
    aflInverse[2][0] =
        (aflWorld[1][0] * aflWorld[2][1] - aflWorld[2][0] * aflWorld[1][1]) * flScale;
    aflInverse[2][1] =
        (aflWorld[2][0] * aflWorld[0][1] - aflWorld[0][0] * aflWorld[2][1]) * flScale;
    aflInverse[2][2] =
        (aflWorld[0][0] * aflWorld[1][1] - aflWorld[1][0] * aflWorld[0][1]) * flScale;

    // The inverse translation is the negated world translation run through the inverse basis.
    Vector3 negated;
    NegateVec3(&aflWorld[3][0], &negated.x);
    aflInverse[3][0] =
        aflInverse[0][0] * negated.x + aflInverse[1][0] * negated.y + aflInverse[2][0] * negated.z;
    aflInverse[3][1] =
        aflInverse[0][1] * negated.x + aflInverse[1][1] * negated.y + aflInverse[2][1] * negated.z;
    aflInverse[3][2] =
        aflInverse[0][2] * negated.x + aflInverse[1][2] * negated.y + aflInverse[2][2] * negated.z;
}

// 0x0048c138
// Versions 1 through 3 stored a run of vertex indices per record. The renderer no longer uses
// them, and the loader releases the vector as soon as it has been read.
Stream &ReadIndexRunVector(Stream &stream, std::vector<std::vector<unsigned short> > &runs) {
    int nCount = 0;
    stream.Read(&nCount, sizeof(nCount));
    runs.resize(nCount);
    for (auto &run : runs) {
        int nIndexCount = 0;
        stream.Read(&nIndexCount, sizeof(nIndexCount));
        run.resize(nIndexCount);
        for (auto &index : run) {
            stream.Read(&index, sizeof(index));
        }
    }
    return stream;
}

} // namespace

// 0x00492ff0
Mesh *NewMesh(const HxStr &name) {
    // The binary bills the allocation to the tag "Rnd::Mesh" and rounds the 0x16c-byte object up
    // to 0x170 bytes.
    return new Mesh(name);
}

// 0x006eed60
Mesh *(*g_pfnNewMesh)(const HxStr &name) = NewMesh;

// 0x00492f50
Object *CreateRegisteredMesh(const HxStr &name) {
    return g_pfnNewMesh(name);
}

// 0x004926b0
void RegisterMeshClass() {
    g_pfnNewMesh = NewMesh;
    g_manager.RegisterClass(g_meshClassName, CreateRegisteredMesh);
}

// 0x006eed68
HxStr g_meshClassName("Mesh");

// 0x00894d68
int g_nRndMeshLoadVersion;

// 0x0047ff20
Mesh::Mesh(const HxStr &name)
    : Object(name), mZMode(kZModeZReadWrite), mZFunc(kZFuncLess), mMat(nullptr), mVertsOwner(this),
      mFacesOwner(this), mTransOwner(this), mTrans1Owner(nullptr), mTrans2Owner(nullptr),
      mMaxVerts(-1), mMinScreen(0.0f), mNext(nullptr) {
    // The centre is written as one quadword, which sets the padding word to 1.0 along with it.
    mSphere.mCenter.x = 0.0f;
    mSphere.mCenter.y = 0.0f;
    mSphere.mCenter.z = 0.0f;
    mSphere.mCenter.w = 1.0f;
    mSphere.mRadius = 0.0f;
}

// 0x00492838
Mesh::~Mesh() {
    RemoveObjectRefs();
    ReleaseAllRefs();
}

// 0x00480d80
void Mesh::DumpText(FailSink &sink) {
    Object::DumpText(sink);
    Transformable::DumpText(sink);
    Drawable::DumpText(sink);
    Collideable::DumpText(sink);
    if (sink.mDumpLevel <= 0) {
        return;
    }

    sink.Print("[Mesh]\n");
    sink.Print("zMode:");
    PrintZMode(sink, mZMode);
    sink.Print(" zFunc:");
    PrintZFunc(sink, mZFunc);
    sink.Print(" mat:");
    PrintObjectRef(sink, mMat);
    sink.Print("\n");

    sink.Print("vertsOwner:");
    PrintObjectRef(sink, mVertsOwner);
    sink.Print(" facesOwner:");
    PrintObjectRef(sink, mFacesOwner);
    sink.Print("\n");

    sink.Print("transOwner:");
    PrintObjectRef(sink, mTransOwner);
    sink.Print(" trans1Owner:");
    PrintObjectRef(sink, mTrans1Owner);
    sink.Print("\n");

    sink.Print("trans2Owner:");
    PrintObjectRef(sink, mTrans2Owner);
    sink.Print(" sphere:");
    sink.Print("\n\tcenter:");
    PrintVector3(sink, mSphere.mCenter);
    sink.Print(" radius:");
    sink.Format("%.2f", mSphere.mRadius);
    sink.Print("\n");

    sink.Print("next:");
    PrintObjectRef(sink, mNext);
    sink.Print(" minScreen:");
    sink.Format("%.2f", mMinScreen);
    sink.Print(" maxVerts:");
    sink.Format("%d", mMaxVerts);
    sink.Print("\n");

    sink.Print("verts:");
    DumpVertVector(sink, mVerts);
    sink.Print("\n");
    sink.Print("faces:");
    DumpFaceVector(sink, mFaces);
    sink.Print("\n");
    sink.Print("edges:");
    DumpEdgeVector(sink, mEdges);
    sink.Print("\n");
}

// 0x00481300
void Mesh::Save(Stream &stream) {
    const int nVersion = kSerialVersion;
    stream.Write(&nVersion, sizeof(nVersion));

    Transformable::Save(stream);
    Drawable::Save(stream);
    Collideable::Save(stream);

    stream.Write(&mZMode, sizeof(mZMode));
    stream.Write(&mZFunc, sizeof(mZFunc));
    WriteObjectRef(stream, mMat);

    WriteObjectRef(stream, mVertsOwner);
    WriteObjectRef(stream, mFacesOwner);
    WriteObjectRef(stream, mTransOwner);
    WriteObjectRef(stream, mTrans1Owner);
    WriteObjectRef(stream, mTrans2Owner);

    stream.Write(&mSphere.mCenter.x, sizeof(float));
    stream.Write(&mSphere.mCenter.y, sizeof(float));
    stream.Write(&mSphere.mCenter.z, sizeof(float));
    stream.Write(&mSphere.mRadius, sizeof(mSphere.mRadius));

    WriteObjectRef(stream, mNext);
    stream.Write(&mMinScreen, sizeof(mMinScreen));
    stream.Write(&mMaxVerts, sizeof(mMaxVerts));

    WriteVertVector(stream, mVerts);
    WriteFaceVector(stream, mFaces);
    WriteEdgeVector(stream, mEdges);
}

// 0x00482810
void Mesh::Replace(Object *pFrom, Object *pTo) {
    Transformable::Replace(pFrom, pTo);
    Drawable::Replace(pFrom, pTo);
    Collideable::Replace(pFrom, pTo);

    if (mMat == pFrom && mMat != nullptr) {
        pFrom->RemoveRef(this);
        mMat = dynamic_cast<Mat *>(pTo);
        if (mMat != nullptr) {
            mMat->AddRef(this);
        }
    }
    if (mNext == pFrom && mNext != nullptr) {
        pFrom->RemoveRef(this);
        mNext = dynamic_cast<Mesh *>(pTo);
        if (mNext != nullptr) {
            mNext->AddRef(this);
        }
    }

    // Losing the mesh that owned the shared geometry takes a copy of the geometry rather than
    // discarding it.
    if (pTo != nullptr) {
        if (mVertsOwner == pFrom && mVertsOwner != nullptr) {
            pFrom->RemoveRef(this);
            mVertsOwner = dynamic_cast<Mesh *>(pTo);
            if (mVertsOwner != nullptr) {
                mVertsOwner->AddRef(this);
            }
        }
    } else if (mVertsOwner == pFrom && mVertsOwner != nullptr) {
        mVerts = mVertsOwner->mVerts;
        mVertsOwner = this;
        Sync();
    }

    if (pTo != nullptr) {
        if (mFacesOwner == pFrom && mFacesOwner != nullptr) {
            pFrom->RemoveRef(this);
            mFacesOwner = dynamic_cast<Mesh *>(pTo);
            if (mFacesOwner != nullptr) {
                mFacesOwner->AddRef(this);
            }
        }
    } else if (mFacesOwner == pFrom && mFacesOwner != nullptr) {
        mFaces = mFacesOwner->mFaces;
        mFacesOwner = this;
        Sync();
    }

    // Losing the transform owner adopts the owner's world position as this mesh's own local
    // transform, so the mesh stays where it was drawn.
    if (pTo != nullptr) {
        if (mTransOwner == pFrom && mTransOwner != nullptr) {
            pFrom->RemoveRef(this);
            mTransOwner = dynamic_cast<Transformable *>(pTo);
            if (mTransOwner != nullptr) {
                mTransOwner->AddRef(this);
            }
        }
    } else if (mTransOwner == pFrom && mTransOwner != nullptr) {
        // The compiler inlined the whole adoption here. In order, the departing owner's world
        // transform becomes this mesh's local transform, a recompose follows, then the owner's
        // local transform, billboard mode, and origin row are taken, and a second recompose
        // finishes. Every step reads the other transformable's own fields, so the sequence is
        // Transformable's work rather than the mesh's.
        Transformable::AdoptXfmFrom(*mTransOwner);
        mTransOwner = this;
    }

    if (mTrans1Owner == pFrom && mTrans1Owner != nullptr) {
        pFrom->RemoveRef(this);
        mTrans1Owner = dynamic_cast<Transformable *>(pTo);
        if (mTrans1Owner != nullptr) {
            mTrans1Owner->AddRef(this);
        }
    }
    if (mTrans2Owner == pFrom && mTrans2Owner != nullptr) {
        pFrom->RemoveRef(this);
        mTrans2Owner = dynamic_cast<Transformable *>(pTo);
        if (mTrans2Owner != nullptr) {
            mTrans2Owner->AddRef(this);
        }
    }
}

// 0x00492f00
const HxStr &Mesh::ClassName() const {
    return g_meshClassName;
}

// 0x00482568
void Mesh::Copy(const Object *pSource, unsigned nFlags) {
    const Mesh *pMesh = dynamic_cast<const Mesh *>(pSource);

    Transformable::Copy(pSource, nFlags);
    Drawable::Copy(pSource, nFlags);
    Collideable::Copy(pSource, nFlags);
    RemoveObjectRefs();

    mZMode = pMesh->mZMode;
    mZFunc = pMesh->mZFunc;
    mMat = pMesh->mMat;
    mSphere = pMesh->mSphere;
    mNext = pMesh->mNext;
    mMinScreen = pMesh->mMinScreen;

    if ((nFlags & kCopyShareVerts) != 0) {
        mVertsOwner = pMesh->mVertsOwner;
    } else {
        mMaxVerts = pMesh->mMaxVerts;
        if (pMesh->mVertsOwner == pMesh) {
            mVertsOwner = this;
            mVerts = pMesh->mVerts;
        } else {
            mVertsOwner = pMesh->mVertsOwner;
        }
    }

    if ((nFlags & kCopyShareFaces) != 0) {
        mFacesOwner = pMesh->mFacesOwner;
    } else if (pMesh->mFacesOwner == pMesh) {
        mFacesOwner = this;
        mFaces = pMesh->mFaces;
        mEdges = pMesh->mEdges;
    } else {
        mFacesOwner = pMesh->mFacesOwner;
    }
    ClearSharedGeometry();

    if ((nFlags & kCopyShareTransforms) != 0) {
        mTransOwner = pMesh->mTransOwner;
        mTrans1Owner = pMesh->mTrans1Owner;
        mTrans2Owner = pMesh->mTrans2Owner;
    } else {
        mTransOwner = pMesh->mTransOwner == pMesh ? this : pMesh->mTransOwner;
        mTrans1Owner = pMesh->mTrans1Owner == pMesh ? this : pMesh->mTrans1Owner;
        mTrans2Owner = pMesh->mTrans2Owner == pMesh ? this : pMesh->mTrans2Owner;
    }

    Refresh();
}

// 0x004817d0
void Mesh::Load(Stream &stream) {
    stream.Read(&g_nRndMeshLoadVersion, sizeof(g_nRndMeshLoadVersion));
    if (g_nRndMeshLoadVersion > kSerialVersion) {
        g_failSink.Report("Can't load new Mesh\n");
        g_failSink.mAbortProc();
        return;
    }

    Transformable::Load(stream);
    Drawable::Load(stream);
    Collideable::Load(stream);
    RemoveObjectRefs();

    stream.Read(&mZMode, sizeof(mZMode));
    stream.Read(&mZFunc, sizeof(mZFunc));
    if (g_nRndMeshLoadVersion < 3) {
        // A mesh used to store the transform billboard mode itself.
        int nBillboard = 0;
        stream.Read(&nBillboard, sizeof(nBillboard));
        Transformable::SetBillboard(nBillboard);
    }

    ReadObjectRef(stream, mMat);
    ReadObjectRef(stream, mVertsOwner);
    ReadObjectRef(stream, mFacesOwner);
    ReadObjectRef(stream, mTransOwner);
    ReadObjectRef(stream, mTrans1Owner);
    ReadObjectRef(stream, mTrans2Owner);

    if (g_nRndMeshLoadVersion < 3) {
        // A mesh used to store the transform origin itself.
        Vector3 origin;
        origin.w = 1.0f;
        stream.Read(&origin.x, sizeof(float));
        stream.Read(&origin.y, sizeof(float));
        stream.Read(&origin.z, sizeof(float));
        Transformable::SetOrigin(&origin.x);
    }

    stream.Read(&mSphere.mCenter.x, sizeof(float));
    stream.Read(&mSphere.mCenter.y, sizeof(float));
    stream.Read(&mSphere.mCenter.z, sizeof(float));
    stream.Read(&mSphere.mRadius, sizeof(mSphere.mRadius));

    bool bKeepEdges = true;
    if (g_nRndMeshLoadVersion >= 5 && g_nRndMeshLoadVersion <= 7) {
        char chKeepEdges = 0;
        stream.ReadBytes(&chKeepEdges, sizeof(chKeepEdges));
        bKeepEdges = chKeepEdges != 0;
    }

    if (g_nRndMeshLoadVersion >= 6) {
        ReadObjectRef(stream, mNext);
        stream.Read(&mMinScreen, sizeof(mMinScreen));
    }
    if (g_nRndMeshLoadVersion == 7) {
        char chUnused = 0;
        stream.ReadBytes(&chUnused, sizeof(chUnused));
    }
    if (g_nRndMeshLoadVersion >= 9) {
        stream.Read(&mMaxVerts, sizeof(mMaxVerts));
    }

    ReadVertVector(stream, mVerts);
    ReadFaceVector(stream, mFaces);
    if (g_nRndMeshLoadVersion >= 5) {
        ReadEdgeVector(stream, mEdges);
    }
    if (!bKeepEdges) {
        mEdges.clear();
    }

    if (g_nRndMeshLoadVersion >= 1 && g_nRndMeshLoadVersion <= 3) {
        std::vector<std::vector<unsigned short> > indexRuns;
        ReadIndexRunVector(stream, indexRuns);
    }
    if (g_nRndMeshLoadVersion == 0) {
        char chUnused = 0;
        stream.ReadBytes(&chUnused, sizeof(chUnused));
        float aflUnused[4];
        stream.Read(&aflUnused[0], sizeof(float));
        stream.Read(&aflUnused[1], sizeof(float));
        stream.Read(&aflUnused[2], sizeof(float));
        stream.Read(&aflUnused[3], sizeof(float));
    }

    ClearSharedGeometry();
    Refresh();
}

// 0x00492770
void Mesh::Sync() {
}

// 0x00492778
void Mesh::SyncChanged(int nMask) {
}

// 0x00492780
void Mesh::SyncAll() {
    SyncChanged(kSyncAllMask);
}

// 0x00493e10
void Mesh::Refresh() {
    AddObjectRefs();
    SyncAll();
    Sync();
}

// 0x0047f950
void Mesh::Collide(const Ray &ray, HitSink &sink) {
    if (Drawable::mShowing == 0) {
        return;
    }

    // A sphere of zero radius stands for no bound at all and skips straight to the faces.
    if (mSphere.mRadius != 0.0f) {
        Sphere worldSphere;
        TransformPoint(mTransOwner->mWorldXfm, &mSphere.mCenter.x, &worldSphere.mCenter.x);
        worldSphere.mRadius = mSphere.mRadius;
        float flSphereDistance = 0.0f;
        if (!TestRayAgainstSphere(ray, worldSphere, &flSphereDistance)) {
            return;
        }
    }

    // The faces are tested in local space, so the ray is brought there rather than every vertex
    // being brought out.
    float aflInverse[kXfmRowCount][kXfmRowFloatCount];
    InvertXfm(mTransOwner->mWorldXfm, aflInverse);
    Ray localRay;
    TransformPoint(aflInverse, ray.mStart, localRay.mStart);
    TransformPoint(aflInverse, ray.mEnd, localRay.mEnd);

    // Yes, a mesh with no material tests as though the winding were clockwise, because the
    // binary passes a zero cull mode rather than skipping the facing test.
    const Mat::CullMode nCull = mMat != nullptr ? mMat->mCull : Mat::kCullModeCw;
    const std::vector<MeshVert> &verts = mVertsOwner->mVerts;
    for (const auto &face : mFacesOwner->mFaces) {
        // The first vertex moves as a whole quadword, padding word included.
        TriangleTest tri;
        tri.mVertex[0] = verts[face.mV1].mPoint.x;
        tri.mVertex[1] = verts[face.mV1].mPoint.y;
        tri.mVertex[2] = verts[face.mV1].mPoint.z;
        tri.mVertex[3] = verts[face.mV1].mPoint.w;
        Vec3Sub(&verts[face.mV2].mPoint.x, &verts[face.mV1].mPoint.x, tri.mEdge1);
        Vec3Sub(&verts[face.mV3].mPoint.x, &verts[face.mV1].mPoint.x, tri.mEdge2);
        Vec3Cross(tri.mEdge1, tri.mEdge2, tri.mNormal);

        float flDistance = 0.0f;
        if (TestRayAgainstTriangle(localRay, tri, nCull, &flDistance)) {
            Hit hit;
            hit.mObject = this;
            hit.mDistance = flDistance;
            sink.mHits.push_back(hit);
        }
    }

    // The children are tested after this mesh's own faces.
    Collideable::Collide(ray, sink);
}

// 0x00493a78
void Mesh::SetMaterial(Mat *pMat) {
    if (mMat != nullptr) {
        mMat->RemoveRef(this);
    }
    // Yes, the binary stores the new material only when it is not null.
    if (pMat != nullptr) {
        mMat = pMat;
        pMat->AddRef(this);
    }
}

// 0x00493c40
void Mesh::SetTransOwner(Transformable *pOwner) {
    if (mTransOwner != nullptr) {
        mTransOwner->RemoveRef(this);
    }
    // Yes, the binary stores the new owner only when it is not null.
    if (pOwner != nullptr) {
        mTransOwner = pOwner;
        pOwner->AddRef(this);
    }
}

// Inlined at both of its call sites inside Rnd::MultiMesh::DrawSelf.
void Mesh::SetNext(Mesh *pNext) {
    if (mNext != nullptr) {
        mNext->RemoveRef(this);
    }
    // Yes, the binary stores the new link only when it is not null.
    if (pNext != nullptr) {
        mNext = pNext;
        pNext->AddRef(this);
    }
}

// Inlined as the first half of 0x00493e10.
void Mesh::AddObjectRefs() {
    if (mNext != nullptr) {
        mNext->AddRef(this);
    }
    if (mMat != nullptr) {
        mMat->AddRef(this);
    }
    if (mVertsOwner != nullptr) {
        mVertsOwner->AddRef(this);
    }
    if (mFacesOwner != nullptr) {
        mFacesOwner->AddRef(this);
    }
    if (mTransOwner != nullptr) {
        mTransOwner->AddRef(this);
    }
    if (mTrans1Owner != nullptr) {
        mTrans1Owner->AddRef(this);
    }
    if (mTrans2Owner != nullptr) {
        mTrans2Owner->AddRef(this);
    }
}

// 0x00493d48
void Mesh::RemoveObjectRefs() {
    if (mNext != nullptr) {
        mNext->RemoveRef(this);
    }
    if (mMat != nullptr) {
        mMat->RemoveRef(this);
    }
    if (mVertsOwner != nullptr) {
        mVertsOwner->RemoveRef(this);
    }
    if (mFacesOwner != nullptr) {
        mFacesOwner->RemoveRef(this);
    }
    if (mTransOwner != nullptr) {
        mTransOwner->RemoveRef(this);
    }
    if (mTrans1Owner != nullptr) {
        mTrans1Owner->RemoveRef(this);
    }
    if (mTrans2Owner != nullptr) {
        mTrans2Owner->RemoveRef(this);
    }
}

// 0x0047fe68
void Mesh::ClearSharedGeometry() {
    if (mVertsOwner != this) {
        mVerts.clear();
    }
    if (mFacesOwner != this) {
        mFaces.clear();
        mEdges.clear();
    }
}

} // namespace Rnd
