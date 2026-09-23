#include "rnd/string.h"

#include <list>
#include <math.h>
#include <vector>

#include "math/color.h"
#include "math/vector3.h"
#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/collideable.h"
#include "rnd/drawable.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"
#include "rnd/meshface.h"
#include "rnd/meshvert.h"
#include "rnd/object.h"
#include "rnd/stream.h"
#include "rnd/transformable.h"

namespace Rnd {

namespace {

// The only version this build writes. Load() rejects 3 and above, and the reader therefore accepts
// one version beyond the highest the image can produce.
constexpr int kStringVersion = 2;

// Lowest version Load() refuses.
constexpr int kStringRejectedVersion = 3;

// Version that added mFoldAngle and mHasCaps.
constexpr int kStringFoldVersion = 1;

// Version that added mLinePairs.
constexpr int kStringLinePairsVersion = 2;

// Dump level at or above which DumpText() adds the mesh name.
constexpr int kStringMeshDumpLevel = 2;

// Default fold angle. The constant in the image is 0x3fc90fda, one unit in the last place below
// the nearest float to half of pi, and the decimal literal below is what rounds to it.
constexpr float kDefaultFoldAngle = 1.5707963f;

// Texture coordinate along the ribbon at a start cap, across the body, and at an end cap.
constexpr float kCapStartU = 0.0f;
constexpr float kBodyU = 0.5f;
constexpr float kCapEndU = 1.0f;

// Texture coordinate across the ribbon, one edge to the other.
constexpr float kEdgeFarV = 1.0f;
constexpr float kEdgeNearV = 0.0f;

// Points in one line pair, vertices in one rung of the quad strip, triangles in one quad, and the
// two extra vertices a cap adds at each end.
constexpr int kPointsPerPair = 2;
constexpr int kVertsPerRung = 2;
constexpr int kFacesPerQuad = 2;
constexpr int kCapVerts = 4;

// A capped line pair is three quads over eight vertices.
constexpr int kFacesPerCappedPair = 6;
constexpr int kVertsPerCappedPair = 8;

constexpr char kCountFormat[] = "%u";
constexpr char kIndexFormat[] = "%d";
constexpr char kFloatFormat[] = "%.2f";
constexpr char kQuotedTextFormat[] = "\"%s\"";

// An empty HxStr stores a null buffer, and the binary substitutes the program-wide empty-string
// pointer at 0x006fbd10 rather than passing null to the formatter.
const char *NameText(const Object *pObject) {
    return pObject->mName.mStr != nullptr ? pObject->mName.mStr : "";
}

void PrintObjectName(FailSink &sink, const Object *pObject) {
    if (pObject != nullptr) {
        sink.Format(kQuotedTextFormat, NameText(pObject));
    } else {
        sink.Print("no object");
    }
}

void PrintFlag(FailSink &sink, int nFlag) {
    sink.Print(nFlag != 0 ? "true" : "false");
}

// The material is written as its name including the terminator. An absent material writes one zero
// byte, the empty name a reader resolves to nothing.
void WriteObjectName(Stream &stream, const Object *pObject) {
    if (pObject != nullptr) {
        stream.WriteBytes(NameText(pObject), pObject->mName.mLen + 1);
    } else {
        const char cEmpty = 0;
        stream.WriteBytes(&cEmpty, sizeof(cEmpty));
    }
}

// 0x004be168
// Writes the point count and then one line per point, its index, its position, and
// its colour.
FailSink &DumpPointVector(FailSink &sink, const std::vector<String::Point> &points) {
    sink.Print("(size:");
    sink.Format(kCountFormat, static_cast<unsigned>(points.size()));
    sink.Print(")");
    for (unsigned nIndex = 0; nIndex < points.size(); ++nIndex) {
        sink.Print("\n");
        sink.Format(kIndexFormat, nIndex);
        sink.Print("\t");
        sink.Print("\n\tv:");
        sink.Print("(x:");
        sink.Format(kFloatFormat, points[nIndex].mPos.x);
        sink.Print(" y:");
        sink.Format(kFloatFormat, points[nIndex].mPos.y);
        sink.Print(" z:");
        sink.Format(kFloatFormat, points[nIndex].mPos.z);
        sink.Print(")");
        sink.Print("\n\tc:");
        sink.Print("(r:");
        sink.Format(kFloatFormat, points[nIndex].mColor.r);
        sink.Print(" g:");
        sink.Format(kFloatFormat, points[nIndex].mColor.g);
        sink.Print(" b:");
        sink.Format(kFloatFormat, points[nIndex].mColor.b);
        sink.Print(" a:");
        sink.Format(kFloatFormat, points[nIndex].mColor.a);
        sink.Print(")");
    }
    return sink;
}

// 0x004be408
// Writes the point count and then seven floats per point. The padding word of the
// position never arrives at a file.
Stream &WritePointVector(Stream &stream, const std::vector<String::Point> &points) {
    const int nCount = static_cast<int>(points.size());
    stream.Write(&nCount, sizeof(nCount));
    for (unsigned nIndex = 0; nIndex < points.size(); ++nIndex) {
        stream.Write(&points[nIndex].mPos.x, sizeof(float));
        stream.Write(&points[nIndex].mPos.y, sizeof(float));
        stream.Write(&points[nIndex].mPos.z, sizeof(float));
        stream.Write(&points[nIndex].mColor.r, sizeof(float));
        stream.Write(&points[nIndex].mColor.g, sizeof(float));
        stream.Write(&points[nIndex].mColor.b, sizeof(float));
        stream.Write(&points[nIndex].mColor.a, sizeof(float));
    }
    return stream;
}

// 0x004be5a8
// The reader counterpart of WritePointVector(). The resize fills every new element
// with a default-constructed point before the seven floats overwrite its first two members.
Stream &ReadPointVector(Stream &stream, std::vector<String::Point> &points) {
    int nCount = 0;
    stream.Read(&nCount, sizeof(nCount));
    points.resize(nCount);
    for (unsigned nIndex = 0; nIndex < points.size(); ++nIndex) {
        stream.Read(&points[nIndex].mPos.x, sizeof(float));
        stream.Read(&points[nIndex].mPos.y, sizeof(float));
        stream.Read(&points[nIndex].mPos.z, sizeof(float));
        stream.Read(&points[nIndex].mColor.r, sizeof(float));
        stream.Read(&points[nIndex].mColor.g, sizeof(float));
        stream.Read(&points[nIndex].mColor.b, sizeof(float));
        stream.Read(&points[nIndex].mColor.a, sizeof(float));
    }
    return stream;
}

} // namespace

// 0x006fc348
HxStr g_stringClassName("String");

// 0x004bf440
// The creator the class registry stores. NewString() is inlined into it, and the null
// test the compiler emits there is the conversion of a Rnd::String pointer to its Rnd::Object
// virtual base rather than a check the source requests.
static Object *NewStringObject(const HxStr &name) {
    return String::NewString(name);
}

String::Point::Point() {
    mPos.x = 0.0f;
    mPos.y = 0.0f;
    mPos.z = 0.0f;
    mPos.w = 1.0f;
    mColor.r = 1.0f;
    mColor.g = 1.0f;
    mColor.b = 1.0f;
    mColor.a = 1.0f;
    // Only the padding word of the camera-space position is written. The rest of the frame scratch
    // stays indeterminate until DrawSelf() fills it, matching the binary.
    mCamPos.w = 1.0f;
}

String::String(const HxStr &name)
    : Object(name), mpMat(nullptr), mWidth(1.0f), mHasCaps(1), mLinePairs(0),
      mFoldAngle(kDefaultFoldAngle) {
    CreateMesh();
}

String::~String() {
    DeleteMesh();
    ReleaseAllRefs();
}

void String::ResolvePointVertexSlot(unsigned nIndex, VertexSlot &slot) {
    std::vector<MeshVert> &verts = mpMesh->mVertsOwner->mVerts;

    if (mHasCaps == 0) {
        slot.mMode = kVertexSlotBody;
        slot.mpVert = &verts[nIndex * kVertsPerRung];
        return;
    }

    if (mLinePairs != 0) {
        slot.mMode = (nIndex & 1) != 0 ? kVertexSlotEndCap : kVertexSlotStartCap;
        slot.mpVert = &verts[nIndex * kCapVerts];
        return;
    }

    if (nIndex == 0) {
        slot.mMode = kVertexSlotStartCap;
        slot.mpVert = &verts[0];
        return;
    }

    if (nIndex == mPoints.size() - 1) {
        slot.mMode = kVertexSlotEndCap;
        slot.mpVert = &verts[verts.size() - kCapVerts];
        return;
    }

    // A middle point steps past the two extra vertices the start cap occupies.
    slot.mMode = kVertexSlotBody;
    slot.mpVert = &verts[(nIndex + 1) * kVertsPerRung];
}

void String::SetNumPoints(int nCount) {
    mPoints.resize(nCount);
    if (nCount <= 0) {
        return;
    }

    int nRungs = nCount;
    if (mHasCaps != 0) {
        nRungs = mLinePairs != 0 ? (nCount / kPointsPerPair) * kCapVerts : nCount + kPointsPerPair;
    }
    mpMesh->mVertsOwner->mVerts.resize(nRungs * kVertsPerRung);

    for (unsigned nIndex = 0; nIndex < mPoints.size(); ++nIndex) {
        VertexSlot slot;
        ResolvePointVertexSlot(nIndex, slot);

        if (slot.mMode == kVertexSlotStartCap) {
            slot.mpVert->mTex1.x = kCapStartU;
            slot.mpVert->mTex1.y = kEdgeFarV;
            slot.mpVert->mColor = mPoints[nIndex].mColor;
            ++slot.mpVert;
            slot.mpVert->mTex1.x = kCapStartU;
            slot.mpVert->mTex1.y = kEdgeNearV;
            slot.mpVert->mColor = mPoints[nIndex].mColor;
            ++slot.mpVert;
        }

        slot.mpVert->mTex1.x = kBodyU;
        slot.mpVert->mTex1.y = kEdgeFarV;
        slot.mpVert->mColor = mPoints[nIndex].mColor;
        ++slot.mpVert;
        slot.mpVert->mTex1.x = kBodyU;
        slot.mpVert->mTex1.y = kEdgeNearV;
        slot.mpVert->mColor = mPoints[nIndex].mColor;
        ++slot.mpVert;

        if (slot.mMode == kVertexSlotEndCap) {
            slot.mpVert->mTex1.x = kCapEndU;
            slot.mpVert->mTex1.y = kEdgeFarV;
            slot.mpVert->mColor = mPoints[nIndex].mColor;
            ++slot.mpVert;
            slot.mpVert->mTex1.x = kCapEndU;
            slot.mpVert->mTex1.y = kEdgeNearV;
            slot.mpVert->mColor = mPoints[nIndex].mColor;
            ++slot.mpVert;
        }
    }

    mpMesh->SyncChanged(Mesh::kSyncColors | Mesh::kSyncTexs);

    int nFaceCount = (nRungs - 1) * kFacesPerQuad;
    if (mLinePairs != 0) {
        nFaceCount = mHasCaps != 0 ? (nRungs * kFacesPerCappedPair) / kVertsPerCappedPair : nRungs;
    }
    std::vector<MeshFace> &faces = mpMesh->mFacesOwner->mFaces;
    faces.resize(nFaceCount);

    // The strip is written from its far end back towards index 0, two triangles per rung pair.
    for (int nFace = nFaceCount - kFacesPerQuad; nFace >= 0; nFace -= kFacesPerQuad) {
        int nBase = nFace;
        if (mLinePairs != 0) {
            nBase = nFace * kPointsPerPair;
            if (mHasCaps != 0) {
                nBase = (nFace / kFacesPerCappedPair) * kVertsPerCappedPair +
                        nFace % kFacesPerCappedPair;
            }
        }

        faces[nFace].mV1 = nBase;
        faces[nFace].mV2 = nBase + 2;
        faces[nFace].mV3 = nBase + 1;
        faces[nFace + 1].mV1 = nBase + 1;
        faces[nFace + 1].mV2 = nBase + 2;
        faces[nFace + 1].mV3 = nBase + 3;
    }

    mpMesh->Sync();
}

int String::GetNumPoints() const {
    return static_cast<int>(mPoints.size());
}

void String::SetPointPos(int nIndex, const Vector3 &pos) {
    mPoints[nIndex].mPos = pos;
}

Vector3 *String::GetPointPos(int nIndex) {
    return &mPoints[nIndex].mPos;
}

void String::SetPointColor(int nIndex, const Color &color) {
    mPoints[nIndex].mColor = color;

    VertexSlot slot;
    ResolvePointVertexSlot(nIndex, slot);
    slot.mpVert->mColor = color;
    ++slot.mpVert;
    slot.mpVert->mColor = color;
    ++slot.mpVert;
    if (slot.mMode != kVertexSlotBody) {
        slot.mpVert->mColor = color;
        ++slot.mpVert;
        slot.mpVert->mColor = color;
        ++slot.mpVert;
    }

    mpMesh->SyncChanged(Mesh::kSyncColors);
}

Color *String::GetPointColor(int nIndex) {
    return &mPoints[nIndex].mColor;
}

void String::SetMat(Mat *pMat) {
    mpMesh->SetMaterial(pMat);
}

Mat *String::GetMat() const {
    return mpMesh->mMat;
}

float String::GetWidth() const {
    return mWidth;
}

void String::SetFoldAngle(float flAngle) {
    mFoldAngle = flAngle;
    mFoldSin = sinf(flAngle);
}

float String::GetFoldAngle() const {
    return mFoldAngle;
}

void String::SetHasCaps(int nHasCaps) {
    mHasCaps = nHasCaps;
    SetNumPoints(GetNumPoints());
}

int String::GetHasCaps() const {
    return mHasCaps;
}

void String::SetLinePairs(int nLinePairs) {
    mLinePairs = nLinePairs;
    SetNumPoints(GetNumPoints());
}

int String::GetLinePairs() const {
    return mLinePairs;
}

void String::SetHighlight(int nHighlight) {
    mpMesh->SetHighlight(nHighlight);
}

void String::Collide(const Ray &ray, HitSink &sink) {
    if (mShowing == 0) {
        return;
    }

    // The position is taken before the mesh is tested. The walk below therefore visits only the
    // hits the mesh appended. On an empty sink it addresses the list sentinel, and the increment
    // still arrives at the first new entry.
    std::list<Hit>::iterator itBefore = sink.mHits.end();
    --itBefore;

    mpMesh->Collide(ray, sink);

    for (std::list<Hit>::iterator it = ++itBefore; it != sink.mHits.end(); ++it) {
        it->mObject = this;
    }

    Collideable::Collide(ray, sink);
}

void String::DumpText(FailSink &sink) {
    Object::DumpText(sink);
    Drawable::DumpText(sink);
    Collideable::DumpText(sink);
    Transformable::DumpText(sink);

    if (sink.mDumpLevel <= 0) {
        return;
    }

    sink.Print("[String]\n");
    sink.Print("points:");
    DumpPointVector(sink, mPoints);
    sink.Print("\n");
    sink.Print("width:");
    sink.Format(kFloatFormat, mWidth);
    sink.Print(" foldAngle:");
    sink.Format(kFloatFormat, mFoldAngle);
    sink.Print(" hasCaps:");
    PrintFlag(sink, mHasCaps);
    sink.Print("\n");
    sink.Print("linePairs:");
    PrintFlag(sink, mLinePairs);
    sink.Print("\n");

    if (sink.mDumpLevel < kStringMeshDumpLevel) {
        return;
    }

    sink.Print("mesh:");
    PrintObjectName(sink, mpMesh);
    sink.Print("\n");
}

void String::Save(Stream &stream) {
    const int nVersion = kStringVersion;
    stream.Write(&nVersion, sizeof(nVersion));

    Drawable::Save(stream);
    Collideable::Save(stream);
    Transformable::Save(stream);

    WriteObjectName(stream, mpMesh->mMat);
    WritePointVector(stream, mPoints);
    stream.Write(&mWidth, sizeof(mWidth));
    stream.Write(&mFoldAngle, sizeof(mFoldAngle));

    const char cHasCaps = mHasCaps;
    stream.WriteBytes(&cHasCaps, sizeof(cHasCaps));
    const char cLinePairs = mLinePairs;
    stream.WriteBytes(&cLinePairs, sizeof(cLinePairs));
}

void String::Replace(Object *pFrom, Object *pTo) {
    Drawable::Replace(pFrom, pTo);
    Collideable::Replace(pFrom, pTo);
    Transformable::Replace(pFrom, pTo);
}

const HxStr &String::ClassName() const {
    return g_stringClassName;
}

void String::Copy(const Object *pSource, unsigned nFlags) {
    const String *pSourceString = dynamic_cast<const String *>(pSource);

    Drawable::Copy(pSource, nFlags);
    Collideable::Copy(pSource, nFlags);
    Transformable::Copy(pSource, nFlags);

    DeleteMesh();

    mpMat = pSourceString->mpMesh->mMat;
    mPoints = pSourceString->mPoints;
    mWidth = pSourceString->mWidth;
    mFoldAngle = pSourceString->mFoldAngle;
    mHasCaps = pSourceString->mHasCaps;
    mLinePairs = pSourceString->mLinePairs;

    CreateMesh();
}

void String::Load(Stream &stream) {
    int nVersion = 0;
    stream.Read(&nVersion, sizeof(nVersion));
    if (nVersion >= kStringRejectedVersion) {
        g_failSink.Report("Can't load new String\n");
        return;
    }

    Drawable::Load(stream);
    Collideable::Load(stream);
    Transformable::Load(stream);

    DeleteMesh();

    HxStr matName(nullptr);
    stream.ReadString(matName);
    mpMat = dynamic_cast<Mat *>(g_manager.Find(matName));

    ReadPointVector(stream, mPoints);
    stream.Read(&mWidth, sizeof(mWidth));

    if (nVersion >= kStringFoldVersion) {
        stream.Read(&mFoldAngle, sizeof(mFoldAngle));
        char cHasCaps = 0;
        stream.ReadBytes(&cHasCaps, sizeof(cHasCaps));
        mHasCaps = cHasCaps != 0;
    }

    if (nVersion >= kStringLinePairsVersion) {
        char cLinePairs = 0;
        stream.ReadBytes(&cLinePairs, sizeof(cLinePairs));
        mLinePairs = cLinePairs != 0;
    }

    CreateMesh();
}

void String::CreateMesh() {
    // The binary builds the name through two HxStr operator+ calls, each of which copies its left
    // operand, appends to the copy, and returns it. hxstr.h declares neither overload yet. The
    // same three steps therefore appear here against one local.
    HxStr meshName("[");
    meshName += mName;
    meshName += "_mesh]";

    mpMesh = g_pfnNewMesh(meshName);
    mpMesh->mInternal = 1;
    mpMesh->SetMaterial(mpMat);
    mpMesh->mZMode = Mesh::kZModeZReadOnly;
    mpMesh->mZFunc = Mesh::kZFuncLess;
    mFoldSin = sinf(mFoldAngle);
    SetNumPoints(GetNumPoints());
}

void String::DeleteMesh() {
    if (mpMesh != nullptr) {
        delete mpMesh;
    }
    mpMesh = nullptr;
}

// 0x004bedc8
String *String::NewString(const HxStr &name) {
    try {
        return new String(name);
    } catch (...) {
        return nullptr; // The binary's handler returns null.
    }
}

void String::Init() {
    g_manager.RegisterClass(g_stringClassName, NewStringObject);
}

} // namespace Rnd
