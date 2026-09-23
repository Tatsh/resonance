#include "rnd/mesh.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <list>
#include <vector>

#include "math/box.h"
#include "math/color.h"
#include "math/quaternion.h"
#include "math/vector2.h"
#include "math/vector3.h"
#include "netflow/netflow.h"
#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/meshanim.h"
#include "rnd/raytest.h"
#include "rnd/stream.h"
#include "rnd/transformable.h"

namespace Rnd {

namespace {

// The text dump writes an absent object reference as this literal, and a present one as its
// quoted name.
constexpr char kNoObject[] = "no object";

// The last mesh file version whose faces carry a normal.
constexpr int kFaceNormalLastVersion = 0;

// BoundingSphere() places the centre halfway between the box corners.
constexpr float kHalf = 0.5f;

// One corner of the cube MakeCube() builds, each axis at the high or the low extent.
struct CubeCorner {
    bool mHighX;
    bool mHighY;
    bool mHighZ;
};

// The corners in the binary's vertex order.
constexpr CubeCorner kCubeCorners[] = {
    {false, true, false},
    {false, false, false},
    {true, false, false},
    {true, true, false},
    {false, true, true},
    {false, false, true},
    {true, false, true},
    {true, true, true},
};

// The cube's triangles and drawn edges, as indices into kCubeCorners.
constexpr MeshFace kCubeFaces[] = {
    {0, 3, 1},
    {1, 3, 2},
    {4, 5, 7},
    {5, 6, 7},
    {0, 5, 4},
    {0, 1, 5},
    {1, 6, 5},
    {1, 2, 6},
    {7, 6, 2},
    {7, 2, 3},
    {4, 7, 0},
    {0, 7, 3},
};

constexpr MeshEdge kCubeEdges[] = {
    {0, 1},
    {1, 2},
    {2, 3},
    {3, 0},
    {4, 5},
    {5, 6},
    {6, 7},
    {7, 4},
    {0, 4},
    {1, 5},
    {2, 6},
    {3, 7},
};

// The tag every mesh allocation is billed to.
constexpr char kMeshAllocationTag[] = "Rnd::Mesh";

// The rows of a transform, three axes and then the translation.
enum XfmRow {
    kXfmRowX = 0,
    kXfmRowY = 1,
    kXfmRowZ = 2,
    kXfmRowTranslation = 3,
};

constexpr int kFaceCornerCount = 3;

inline float DotVec3(const float *pA, const float *pB) {
    return (pA[0] * pB[0]) + (pA[1] * pB[1]) + (pA[2] * pB[2]);
}

// The inlined VU0 normalise, one rsqrt of the squared length scaling the three components.
inline void NormalizeVec3Inline(const Vector3 &source, Vector3 &result) {
    const float flInverseLength = 1.0f / std::sqrt(DotVec3(&source.x, &source.x));
    result.x = source.x * flInverseLength;
    result.y = source.y * flInverseLength;
    result.z = source.z * flInverseLength;
}

// Rotate a face so that it starts at nVert, keeping its winding. A face without nVert at its
// second or third corner is not changed.
inline void RotateFaceToStart(MeshFace &face, unsigned short nVert) {
    const MeshFace old = face;
    if (face.mV2 == nVert) {
        face = MeshFace{nVert, old.mV3, old.mV1};
    } else if (face.mV3 == nVert) {
        face = MeshFace{nVert, old.mV1, old.mV2};
    }
}

// Point every corner of a face that uses nFrom at nTo instead.
inline void ReplaceCorner(MeshFace &face, int nFrom, int nTo) {
    if (face.mV1 == nFrom) {
        face.mV1 = nTo;
    }
    if (face.mV2 == nFrom) {
        face.mV2 = nTo;
    }
    if (face.mV3 == nFrom) {
        face.mV3 = nTo;
    }
}

inline unsigned short FaceCorner(const MeshFace &face, int nCorner) {
    switch (nCorner) {
    case 0:
        return face.mV1;
    case 1:
        return face.mV2;
    default:
        return face.mV3;
    }
}

inline bool SameVec3(const Vector3 &a, const Vector3 &b) {
    return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
}

inline bool SameColor(const Color &a, const Color &b) {
    return (a.r == b.r) && (a.g == b.g) && (a.b == b.b) && (a.a == b.a);
}

inline bool SameVec2(const Vector2 &a, const Vector2 &b) {
    return (a.x == b.x) && (a.y == b.y);
}

// The vertex MakeCube() and WeldVerts() fill with. The position and normal are the origin with a
// 1.0 fourth word, the colour is opaque white, and both texture coordinates are zero.
inline MeshVert DefaultVert() {
    MeshVert vert;
    vert.mPoint = Vector3{0.0f, 0.0f, 0.0f, 1.0f};
    vert.mNorm = vert.mPoint;
    vert.mColor = Color{1.0f, 1.0f, 1.0f, 1.0f};
    vert.mTex1 = Vector2{0.0f, 0.0f};
    vert.mTex2 = vert.mTex1;
    return vert;
}

// Bits WeldVerts() sets for each vertex a face or an edge uses. A later duplicate's entry is
// replaced with the negated index of the vertex it merges into.
enum VertUse {
    kVertInEdge = 1,
    kVertInFace = 2,
};

// A face whose normal is within two degrees of a neighbour's joins that neighbour's flat fan.
constexpr float kCoplanarCosine = 0.99939f;

// No vertex, in a FlatFace pivot.
constexpr int kNoVert = -1;

constexpr float kOneThird = 1.0f / 3.0f;

// Whether two faces use the same three vertices in any order.
inline bool SameCorners(const MeshFace &a, const MeshFace &b) {
    if (a.mV1 == b.mV1) {
        if ((a.mV2 == b.mV2) && (a.mV3 == b.mV3)) {
            return true;
        }
        if ((a.mV2 == b.mV3) && (a.mV3 == b.mV2)) {
            return true;
        }
    }
    if (a.mV1 == b.mV2) {
        if ((a.mV2 == b.mV3) && (a.mV3 == b.mV1)) {
            return true;
        }
        if ((a.mV2 == b.mV1) && (a.mV3 == b.mV3)) {
            return true;
        }
    }
    if (a.mV1 == b.mV3) {
        if ((a.mV2 == b.mV2) && (a.mV3 == b.mV1)) {
            return true;
        }
        if ((a.mV2 == b.mV1) && (a.mV3 == b.mV2)) {
            return true;
        }
    }
    return false;
}

// Whether every key of a channel matches the first key, value for value.
template <typename Key, typename Same>
bool AllKeysMatchFirst(const std::list<Key> &keys, Same same) {
    for (const Key &key : keys) {
        const Key &first = keys.front();
        if (key.mValues.size() != first.mValues.size()) {
            return false;
        }
        for (size_t i = 0; i < key.mValues.size(); ++i) {
            if (!same(key.mValues[i], first.mValues[i])) {
                return false;
            }
        }
    }
    return true;
}

// Shrink or grow the value vector of every key in a channel by nRemoved values.
template <typename Key, typename Value>
void TrimKeys(std::list<Key> &keys, int nRemoved, const Value &fill) {
    for (Key &key : keys) {
        key.mValues.resize(key.mValues.size() - nRemoved, fill);
    }
}

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

// 0x00482e28
Stream &ReadFace(Stream &stream, MeshFace &face) {
    stream.Read(&face.mV1, sizeof(face.mV1))
        .Read(&face.mV2, sizeof(face.mV2))
        .Read(&face.mV3, sizeof(face.mV3));
    if (g_nRndMeshLoadVersion <= kFaceNormalLastVersion) {
        // Files of these versions store a face normal after the indices, which is discarded.
        Vector3 normal;
        normal.w = 1.0f;
        stream.Read(&normal.x, sizeof(normal.x))
            .Read(&normal.y, sizeof(normal.y))
            .Read(&normal.z, sizeof(normal.z));
    }
    return stream;
}

// 0x0048ae78
Stream &ReadFaceVector(Stream &stream, std::vector<MeshFace> &faces) {
    int nCount = 0;
    stream.Read(&nCount, sizeof(nCount));
    faces.resize(nCount);
    for (auto &face : faces) {
        ReadFace(stream, face);
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
    try {
        return new Mesh(name);
    } catch (...) {
        return nullptr;
    }
}

// 0x00492590
void *Mesh::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, kMeshAllocationTag);
}

// 0x004925b0
void Mesh::operator delete(void *pBlock) {
    FreeTaggedMemory(pBlock, kMeshAllocationTag);
}

// 0x006eed60
Mesh *(*g_pfnNewMesh)(const HxStr &name) = NewMesh;

// 0x004926f0
Mesh *NewMeshThroughHook(const HxStr &name) {
    return g_pfnNewMesh(name);
}

// 0x00492f50
Object *CreateRegisteredMesh(const HxStr &name) {
    try {
        return g_pfnNewMesh(name);
    } catch (...) {
        return nullptr; // The binary's handler returns null.
    }
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
void Mesh::SyncChanged([[maybe_unused]] int nMask) {
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
    mMat = pMat;
    if (pMat != nullptr) {
        pMat->AddRef(this);
    }
}

// 0x00493c98
void Mesh::SetTrans1Owner(Transformable *pOwner) {
    if (mTrans1Owner != nullptr) {
        mTrans1Owner->RemoveRef(this);
    }
    mTrans1Owner = pOwner;
    if (pOwner != nullptr) {
        pOwner->AddRef(this);
    }
}

// 0x00493cf0
void Mesh::SetTrans2Owner(Transformable *pOwner) {
    if (mTrans2Owner != nullptr) {
        mTrans2Owner->RemoveRef(this);
    }
    mTrans2Owner = pOwner;
    if (pOwner != nullptr) {
        pOwner->AddRef(this);
    }
}

// 0x00492e98
Sphere Mesh::WorldSphere() {
    const auto &xfm = mTransOwner->mWorldXfm;
    const Vector3 &center = mSphere.mCenter;
    Sphere sphere;
    sphere.mCenter.x = xfm[kXfmRowX][0] * center.x + xfm[kXfmRowY][0] * center.y +
                       xfm[kXfmRowZ][0] * center.z + xfm[kXfmRowTranslation][0];
    sphere.mCenter.y = xfm[kXfmRowX][1] * center.x + xfm[kXfmRowY][1] * center.y +
                       xfm[kXfmRowZ][1] * center.z + xfm[kXfmRowTranslation][1];
    sphere.mCenter.z = xfm[kXfmRowX][2] * center.x + xfm[kXfmRowY][2] * center.y +
                       xfm[kXfmRowZ][2] * center.z + xfm[kXfmRowTranslation][2];
    sphere.mCenter.w = center.w; // The transform writes three lanes, and the fourth is the input's.
    sphere.mRadius = mSphere.mRadius;
    return sphere;
}

// 0x004940a8
void Mesh::ForwardSync() {
    Sync();
}

// 0x00483030
Sphere Mesh::BoundingSphere() {
    Sphere sphere;
    const std::vector<MeshVert> &verts = mVertsOwner->mVerts;
    if (verts.size() == 0) {
        sphere.mCenter.x = 0.0f;
        sphere.mCenter.y = 0.0f;
        sphere.mCenter.z = 0.0f;
        sphere.mCenter.w = 1.0f;
        sphere.mRadius = 0.0f;
        return sphere;
    }

    // BoundingBox()'s loop, expanded rather than called.
    Box box;
    box.mMin = verts[0].mPoint;
    box.mMax = verts[0].mPoint;
    for (auto it = verts.begin() + 1; it != mVertsOwner->mVerts.end(); ++it) {
        box.GrowToContain(it->mPoint);
    }
    Vector3 sum;
    AddVec3(&box.mMin.x, &box.mMax.x, &sum.x);
    Vector3 center;
    Vec3Scale(&sum.x, kHalf, &center.x);
    sphere.mCenter = center;

    float flRadiusSquared = 0.0f;
    for (auto it = mVertsOwner->mVerts.begin(); it != mVertsOwner->mVerts.end(); ++it) {
        Vector3 offset;
        Vec3Sub(&it->mPoint.x, &sphere.mCenter.x, &offset.x);
        const float flLengthSquared =
            offset.x * offset.x + offset.y * offset.y + offset.z * offset.z;
        flRadiusSquared = std::max(flRadiusSquared, flLengthSquared);
    }
    sphere.mRadius = std::sqrt(flRadiusSquared);

    Vector3 scale;
    Mat33ExtractScale(&mWorldXfm[0][0], &scale.x);
    // Yes, only the third axis scale is made absolute before the three are compared.
    scale.z = std::fabs(scale.z);
    sphere.mRadius *= std::max(scale.x, std::max(scale.y, scale.z));
    return sphere;
}

// 0x004832d0
bool Mesh::JoinFlatFace(FlatFace &primary, FlatFace &face) {
    face.mPrimaryFace = primary.mFace;
    const MeshFace &faceCorners = mFacesOwner->mFaces[face.mFace];
    const MeshFace &primaryCorners = mFacesOwner->mFaces[primary.mFace];
    int nShared = kNoVert;
    int nSharedBefore = kNoVert;
    for (int nCorner = 0; nCorner < kFaceCornerCount; ++nCorner) {
        const int nVert = FaceCorner(primaryCorners, nCorner);
        if ((nVert == faceCorners.mV1) || (nVert == faceCorners.mV2) ||
            (nVert == faceCorners.mV3)) {
            nSharedBefore = nShared;
            nShared = nVert;
        }
    }
    if (nShared == kNoVert) {
        return false;
    }
    if (DotVec3(&primary.mNormal.x, &face.mNormal.x) < kCoplanarCosine) {
        return false;
    }

    if (primary.mSharedEdges == 0) {
        primary.mPivotOther = nSharedBefore;
        primary.mSharedEdges = 1;
        primary.mPivot = nShared;
        return true;
    }
    if ((primary.mPivot == nShared) || (primary.mPivot == nSharedBefore)) {
        primary.mPivotOther = kNoVert;
        ++primary.mSharedEdges;
        return true;
    }
    if ((primary.mSharedEdges == 1) &&
        ((primary.mPivotOther == nShared) || (primary.mPivotOther == nSharedBefore))) {
        primary.mPivot = primary.mPivotOther;
        primary.mPivotOther = kNoVert;
        primary.mSharedEdges = 2;
        return true;
    }
    return false;
}

// 0x00483438
void Mesh::AssignFlatVerts(std::list<MeshAnim *> &anims) {
    std::vector<MeshFace> &faces = mFacesOwner->mFaces;
    std::list<FlatFace> heads;
    std::list<FlatFace> joined;
    for (unsigned nFace = 0; nFace < faces.size(); ++nFace) {
        FlatFace face{};
        face.mFace = nFace;
        joined.push_back(face);

        const std::vector<MeshVert> &verts = mVertsOwner->mVerts;
        Vector3 edge1{};
        Vector3 edge2{};
        Vec3Sub(&verts[faces[nFace].mV2].mPoint.x, &verts[faces[nFace].mV1].mPoint.x, &edge1.x);
        Vec3Sub(&verts[faces[nFace].mV3].mPoint.x, &verts[faces[nFace].mV1].mPoint.x, &edge2.x);
        Vector3 normal{};
        CrossVec3(&edge1.x, &edge2.x, &normal.x);
        NormalizeVec3Inline(normal, joined.back().mNormal);

        std::list<FlatFace>::iterator it = heads.begin();
        for (; it != heads.end(); ++it) {
            if (JoinFlatFace(*it, joined.back())) {
                break;
            }
        }
        if (it == heads.end()) {
            FlatFace head{};
            head.mFace = nFace;
            head.mSharedEdges = 0;
            heads.push_back(head);
            heads.back().mNormal = joined.back().mNormal;
            joined.pop_back();
        }
    }

    netflow_graph graph;
    netflow_graph_init(&graph);
    netflow_v_side side;
    netflow_v_side_init(&side);
    graph.u_count = static_cast<int>(heads.size());
    side.v_count = static_cast<int>(mVertsOwner->mVerts.size());
    graph.edge_count = static_cast<int>(heads.size()) * kFaceCornerCount;
    // Both vertex sets number from 1.
    int nU = 1;
    for (const FlatFace &head : heads) {
        const MeshFace &corners = faces[head.mFace];
        if (head.mSharedEdges == 0) {
            netflow_add_edge(nU, corners.mV1 + 1, &graph, &side);
            netflow_add_edge(nU, corners.mV2 + 1, &graph, &side);
            netflow_add_edge(nU, corners.mV3 + 1, &graph, &side);
        } else {
            netflow_add_edge(nU, head.mPivot + 1, &graph, &side);
            if (head.mPivotOther != kNoVert) {
                netflow_add_edge(nU, head.mPivotOther + 1, &graph, &side);
            }
        }
        ++nU;
    }
    netflow_build_matching(&graph, &side);

    nU = 1;
    for (const FlatFace &head : heads) {
        MeshFace &corners = faces[head.mFace];
        int nVert = graph.u[nU].mate - 1;
        if (nVert < 0) {
            // Unmatched, so the fan splits a vertex of its own.
            nVert = (head.mSharedEdges != 0) ? head.mPivot : corners.mV1;
            mVertsOwner->mVerts.push_back(mVertsOwner->mVerts[nVert]);
            for (MeshAnim *pAnim : anims) {
                pAnim->AppendVertKeys(nVert);
            }
            nVert = static_cast<int>(mVertsOwner->mVerts.size()) - 1;
            if (head.mSharedEdges != 0) {
                ReplaceCorner(corners, head.mPivot, nVert);
                for (const FlatFace &face : joined) {
                    if (face.mPrimaryFace == head.mFace) {
                        ReplaceCorner(faces[face.mFace], head.mPivot, nVert);
                    }
                }
            } else {
                corners.mV1 = nVert;
            }
        }
        RotateFaceToStart(corners, nVert);
        ++nU;
    }
    for (const FlatFace &face : joined) {
        RotateFaceToStart(faces[face.mFace], faces[face.mPrimaryFace].mV1);
    }
    Sync();
}

// 0x00483e70
void Mesh::WeldVerts(bool bAverageColors) {
    if (mVertsOwner->mVerts.empty()) {
        return;
    }

    std::list<MeshAnim *> anims;
    for (Object *pRef : mRefs) {
        MeshAnim *pAnim = dynamic_cast<MeshAnim *>(pRef);
        if ((pAnim != nullptr) && (pAnim->mKeysOwner == pAnim)) {
            anims.push_back(pAnim);
        }
    }

    // Each entry starts as a set of VertUse bits. A merged vertex's entry becomes the negated
    // index of the vertex it merges into, and the compaction below rewrites every entry as the
    // count of vertices removed up to and including that one.
    std::vector<int> vertState(mVertsOwner->mVerts.size(), 0);
    const bool bFlat = (mMat != nullptr) && mMat->mFlat;
    const bool bTextured = (mMat != nullptr) && !mMat->mStages.empty();
    for (const MeshFace &face : mFacesOwner->mFaces) {
        vertState[face.mV1] |= kVertInFace;
        vertState[face.mV2] |= kVertInFace;
        vertState[face.mV3] |= kVertInFace;
    }
    for (const MeshEdge &edge : mFacesOwner->mEdges) {
        vertState[edge.mV1] |= kVertInEdge;
        vertState[edge.mV2] |= kVertInEdge;
    }

    for (unsigned i = 0; i < mVertsOwner->mVerts.size(); ++i) {
        if (vertState[i] <= 0) {
            continue;
        }
        for (unsigned j = i + 1; j < mVertsOwner->mVerts.size(); ++j) {
            if (vertState[j] <= 0) {
                continue;
            }
            const MeshVert &kept = mVertsOwner->mVerts[i];
            const MeshVert &later = mVertsOwner->mVerts[j];
            if (!SameVec3(later.mPoint, kept.mPoint)) {
                continue;
            }
            if (bTextured && !SameVec2(later.mTex1, kept.mTex1)) {
                continue;
            }
            if (!bFlat && (vertState[j] & kVertInFace) && !SameColor(later.mColor, kept.mColor)) {
                continue;
            }
            vertState[j] = -static_cast<int>(i);
        }
    }

    std::vector<Color> faceColors(mFacesOwner->mFaces.size());
    if (bFlat) {
        for (unsigned i = 0; i < mFacesOwner->mFaces.size(); ++i) {
            const MeshFace &face = mFacesOwner->mFaces[i];
            const std::vector<MeshVert> &verts = mVertsOwner->mVerts;
            if (bAverageColors) {
                faceColors[i] = verts[face.mV1].mColor;
                AddColor(faceColors[i], verts[face.mV2].mColor, faceColors[i]);
                AddColor(faceColors[i], verts[face.mV3].mColor, faceColors[i]);
                ScaleColor(faceColors[i], kOneThird, faceColors[i]);
            } else {
                faceColors[i] = verts[face.mV1].mColor;
            }
        }
    }

    // A merged vertex's entry is zero or a negated index, so negating it gives the survivor. An
    // unused vertex is not referenced.
    for (MeshFace &face : mFacesOwner->mFaces) {
        if (vertState[face.mV1] <= 0) {
            face.mV1 = static_cast<unsigned short>(-vertState[face.mV1]);
        }
        if (vertState[face.mV2] <= 0) {
            face.mV2 = static_cast<unsigned short>(-vertState[face.mV2]);
        }
        if (vertState[face.mV3] <= 0) {
            face.mV3 = static_cast<unsigned short>(-vertState[face.mV3]);
        }
    }
    for (MeshEdge &edge : mFacesOwner->mEdges) {
        if (vertState[edge.mV1] <= 0) {
            edge.mV1 = static_cast<unsigned short>(-vertState[edge.mV1]);
        }
        if (vertState[edge.mV2] <= 0) {
            edge.mV2 = static_cast<unsigned short>(-vertState[edge.mV2]);
        }
    }

    for (unsigned i = 0; i < vertState.size(); ++i) {
        const int nRemovedHere = vertState[i] < 1;
        vertState[i] = (i == 0) ? nRemovedHere : (vertState[i - 1] + nRemovedHere);
        if (nRemovedHere == 0) {
            const int nTo = i - vertState[i];
            mVertsOwner->mVerts[nTo] = mVertsOwner->mVerts[i];
            for (MeshAnim *pAnim : anims) {
                pAnim->CopyVertKeys(i, nTo);
            }
        }
    }
    const int nRemoved = vertState.back();
    mVertsOwner->mVerts.resize(mVertsOwner->mVerts.size() - nRemoved, DefaultVert());
    for (MeshAnim *pAnim : anims) {
        MeshAnim *pKeys = pAnim->mKeysOwner;
        TrimKeys(pKeys->mVertPointsKeys, nRemoved, Vector3{0.0f, 0.0f, 0.0f, 1.0f});
        TrimKeys(pKeys->mVertTexsKeys, nRemoved, Vector2{0.0f, 0.0f});
        TrimKeys(pKeys->mVertColorsKeys, nRemoved, Color{0.0f, 0.0f, 0.0f, 1.0f});
    }

    std::vector<MeshFace> &faces = mFacesOwner->mFaces;
    for (std::vector<MeshFace>::iterator it = faces.begin(); it != faces.end();) {
        it->mV1 -= vertState[it->mV1];
        it->mV2 -= vertState[it->mV2];
        it->mV3 -= vertState[it->mV3];
        if ((it->mV1 == it->mV2) || (it->mV2 == it->mV3) || (it->mV3 == it->mV1)) {
            if (!faceColors.empty()) {
                faceColors.erase(faceColors.begin() + (it - faces.begin()));
            }
            it = faces.erase(it);
        } else {
            ++it;
        }
    }
    std::vector<MeshEdge> &edges = mFacesOwner->mEdges;
    for (std::vector<MeshEdge>::iterator it = edges.begin(); it != edges.end();) {
        it->mV1 -= vertState[it->mV1];
        it->mV2 -= vertState[it->mV2];
        if (it->mV1 == it->mV2) {
            it = edges.erase(it);
        } else {
            ++it;
        }
    }

    if (faces.size() >= 2) {
        for (std::vector<MeshFace>::iterator it = faces.begin(); it != faces.end(); ++it) {
            for (std::vector<MeshFace>::iterator later = it + 1; later != faces.end();) {
                if (SameCorners(*later, *it)) {
                    if (!faceColors.empty()) {
                        faceColors.erase(faceColors.begin() + (later - faces.begin()));
                    }
                    later = faces.erase(later);
                } else {
                    ++later;
                }
            }
        }
    }
    if (edges.size() >= 2) {
        for (std::vector<MeshEdge>::iterator it = edges.begin(); it != edges.end(); ++it) {
            for (std::vector<MeshEdge>::iterator later = it + 1; later != edges.end();) {
                if (((later->mV1 == it->mV1) && (later->mV2 == it->mV2)) ||
                    ((later->mV1 == it->mV2) && (later->mV2 == it->mV1))) {
                    later = edges.erase(later);
                } else {
                    ++later;
                }
            }
        }
    }

    for (MeshAnim *pAnim : anims) {
        if (AllKeysMatchFirst(pAnim->mKeysOwner->mVertPointsKeys, SameVec3)) {
            pAnim->mKeysOwner->mVertPointsKeys.clear();
        }
        if (AllKeysMatchFirst(pAnim->mKeysOwner->mVertTexsKeys, SameVec2)) {
            pAnim->mKeysOwner->mVertTexsKeys.clear();
        }
        if (AllKeysMatchFirst(pAnim->mKeysOwner->mVertColorsKeys, SameColor)) {
            pAnim->mKeysOwner->mVertColorsKeys.clear();
        }
    }

    if (bFlat) {
        AssignFlatVerts(anims);
        for (unsigned i = 0; i < faces.size(); ++i) {
            mVertsOwner->mVerts[faces[i].mV1].mColor = faceColors[i];
        }
    }
    SyncAll();
    Sync();
}

// 0x00485978
void Mesh::MakeCube(float flHalfSize) {
    const float flLow = -flHalfSize;
    const float flHigh = flHalfSize;

    mVertsOwner->mVerts.clear();
    mVertsOwner->mVerts.resize(std::size(kCubeCorners), DefaultVert());
    for (size_t i = 0; i < std::size(kCubeCorners); ++i) {
        Vector3 &point = mVertsOwner->mVerts[i].mPoint;
        point.x = kCubeCorners[i].mHighX ? flHigh : flLow;
        point.y = kCubeCorners[i].mHighY ? flHigh : flLow;
        point.z = kCubeCorners[i].mHighZ ? flHigh : flLow;
    }
    SyncAll();

    // The binary clears, fill-resizes, and then stores each record, rather than assigning.
    const MeshFace emptyFace{0, 0, 0};
    mFacesOwner->mFaces.clear();
    mFacesOwner->mFaces.resize(std::size(kCubeFaces), emptyFace);
    std::copy(std::begin(kCubeFaces), std::end(kCubeFaces), mFacesOwner->mFaces.begin());

    const MeshEdge emptyEdge{0, 0};
    mFacesOwner->mEdges.clear();
    mFacesOwner->mEdges.resize(std::size(kCubeEdges), emptyEdge);
    std::copy(std::begin(kCubeEdges), std::end(kCubeEdges), mFacesOwner->mEdges.begin());
    Sync();
}

// 0x00485ef0
void Mesh::ComputeNormals(bool bPositionOnly) {
    Vector3 axisCross{};
    CrossVec3(mWorldXfm[kXfmRowX], mWorldXfm[kXfmRowY], &axisCross.x);
    const bool bMirrored = DotVec3(&axisCross.x, mWorldXfm[kXfmRowZ]) < 0.0f;

    if ((mMat != nullptr) && mMat->mFlat) {
        std::vector<MeshFace>::iterator it = mFacesOwner->mFaces.begin();
        for (; it != mFacesOwner->mFaces.end(); ++it) {
            std::vector<MeshVert> &verts = mVertsOwner->mVerts;
            Vector3 edge1{};
            Vector3 edge2{};
            Vec3Sub(&verts[it->mV2].mPoint.x, &verts[it->mV1].mPoint.x, &edge1.x);
            Vec3Sub(&verts[it->mV3].mPoint.x, &verts[it->mV1].mPoint.x, &edge2.x);
            Vector3 normal{};
            CrossVec3(&edge1.x, &edge2.x, &normal.x);
            NormalizeVec3Inline(normal, mVertsOwner->mVerts[it->mV1].mNorm);
        }
        if (bMirrored) {
            // Yes, the binary negates once after the loop, through the face at end().
            Vector3 &norm = mVertsOwner->mVerts[it->mV1].mNorm;
            NegateVec3(&norm.x, &norm.x);
        }
        return;
    }

    // Each vertex maps to the first earlier vertex it duplicates, or to itself.
    std::vector<int> canonical(mVertsOwner->mVerts.size(), 0);
    for (unsigned i = 0; i < mVertsOwner->mVerts.size(); ++i) {
        const std::vector<MeshVert> &verts = mVertsOwner->mVerts;
        int j = 0;
        for (; j < static_cast<int>(i); ++j) {
            if (SameVec3(verts[j].mPoint, verts[i].mPoint) &&
                (bPositionOnly || SameColor(verts[j].mColor, verts[i].mColor))) {
                break;
            }
        }
        canonical[i] = j;
    }

    // Each vertex takes the angle-weighted sum of the face normals at every corner welded to it.
    for (unsigned i = 0; i < mVertsOwner->mVerts.size(); ++i) {
        Vector3 &norm = mVertsOwner->mVerts[i].mNorm;
        norm.x = 0.0f;
        norm.z = 0.0f;
        norm.y = 0.0f;
        for (unsigned nFace = 0; nFace < mFacesOwner->mFaces.size(); ++nFace) {
            const MeshFace &face = mFacesOwner->mFaces[nFace];
            int nCorner = 0;
            for (; nCorner < kFaceCornerCount; ++nCorner) {
                if (canonical[FaceCorner(face, nCorner)] == canonical[i]) {
                    break;
                }
            }
            if (nCorner == kFaceCornerCount) {
                continue;
            }
            const std::vector<MeshVert> &verts = mVertsOwner->mVerts;
            const Vector3 &corner = verts[FaceCorner(face, nCorner)].mPoint;
            const Vector3 &next = verts[FaceCorner(face, (nCorner + 1) % kFaceCornerCount)].mPoint;
            const Vector3 &prev = verts[FaceCorner(face, (nCorner + 2) % kFaceCornerCount)].mPoint;
            Vector3 edge1{};
            Vector3 edge2{};
            Vec3Sub(&next.x, &corner.x, &edge1.x);
            Vec3Sub(&prev.x, &corner.x, &edge2.x);
            Vector3 faceNormal{};
            CrossVec3(&edge1.x, &edge2.x, &faceNormal.x);
            Vec3Normalize(&faceNormal.x, &faceNormal.x);
            Vec3Normalize(&edge1.x, &edge1.x);
            Vec3Normalize(&edge2.x, &edge2.x);
            Vector3 weighted{};
            Vec3Scale(&faceNormal.x, std::acos(DotVec3(&edge1.x, &edge2.x)), &weighted.x);
            Vector3 &target = mVertsOwner->mVerts[i].mNorm;
            AddVec3(&target.x, &weighted.x, &target.x);
        }
        Vector3 &target = mVertsOwner->mVerts[i].mNorm;
        Vec3Normalize(&target.x, &target.x);
        if (bMirrored) {
            NegateVec3(&target.x, &target.x);
        }
    }
    SyncChanged(kSyncNorms);
}

// 0x00493fb8
Box Mesh::BoundingBox() {
    const std::vector<MeshVert> &verts = mVertsOwner->mVerts;
    Box box;
    box.mMin = verts[0].mPoint;
    box.mMax = verts[0].mPoint;
    for (auto it = verts.begin() + 1; it != mVertsOwner->mVerts.end(); ++it) {
        box.GrowToContain(it->mPoint);
    }
    return box;
}

// 0x00493c40
void Mesh::SetTransOwner(Transformable *pOwner) {
    if (mTransOwner != nullptr) {
        mTransOwner->RemoveRef(this);
    }
    mTransOwner = pOwner;
    if (pOwner != nullptr) {
        pOwner->AddRef(this);
    }
}

// 0x004925d8
void Mesh::SetNext(Mesh *pNext, float flMinScreen) {
    mMinScreen = flMinScreen;
    if (mNext != nullptr) {
        mNext->RemoveRef(this);
    }
    mNext = pNext;
    if (pNext != nullptr) {
        pNext->AddRef(this);
    }
}

// 0x00493b60
void Mesh::SetVertsOwner(Mesh *pOwner) {
    if (mVertsOwner != nullptr) {
        mVertsOwner->RemoveRef(this);
    }
    mVertsOwner = pOwner;
    if (pOwner != nullptr) {
        pOwner->AddRef(this);
    }
    ClearSharedGeometry();
    SyncAll();
}

// 0x00493bd0
void Mesh::SetFacesOwner(Mesh *pOwner) {
    if (mFacesOwner != nullptr) {
        mFacesOwner->RemoveRef(this);
    }
    mFacesOwner = pOwner;
    if (pOwner != nullptr) {
        pOwner->AddRef(this);
    }
    ClearSharedGeometry();
    Sync();
}

// 0x00493ac8
void Mesh::SetMaterialChain(Mat *pMat) {
    if (mMat != nullptr) {
        mMat->RemoveRef(this);
    }
    mMat = pMat;
    if (pMat != nullptr) {
        pMat->AddRef(this);
    }
    if (mNext != nullptr) {
        mNext->SetMaterialChain(pMat);
    }
}

// 0x00493b30
void Mesh::SetDepthChain(ZMode zMode, ZFunc zFunc) {
    mZMode = zMode;
    mZFunc = zFunc;
    if (mNext != nullptr) {
        mNext->SetDepthChain(zMode, zFunc);
    }
}

// 0x00494048
void Mesh::SetVertexColor(const Color &color) {
    for (MeshVert &vert : mVertsOwner->mVerts) {
        vert.mColor = color;
    }
    SyncChanged(kSyncColors);
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
