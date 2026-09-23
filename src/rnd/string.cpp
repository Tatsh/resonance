#include "rnd/string.h"

#include <list>
#include <math.h>
#include <string.h>
#include <vector>

#include "math/color.h"
#include "math/transformops.h"
#include "math/vector2.h"
#include "math/vector3.h"
#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/cam.h"
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

// 0x004ba898
String::String(const HxStr &name)
    : Object(name), mpMat(nullptr), mWidth(1.0f), mHasCaps(1), mLinePairs(0),
      mFoldAngle(kDefaultFoldAngle) {
    CreateMesh();
}

// 0x004beee8
String::~String() {
    DeleteMesh();
    ReleaseAllRefs();
}

// A point is behind the camera when it is closer than this beyond the near plane.
constexpr float kNearPlaneMargin = 0.01f;

// Fewest points DrawSelf() draws a ribbon through.
constexpr unsigned kMinRibbonPoints = 2;

// A point carried through a transform, the VU0 multiply and accumulate the image inlines. The w
// component is taken from the input.
static inline void TransformPoint(const float (&aflXfm)[kXfmRowCount][kXfmRowFloatCount],
                                  const Vector3 &in,
                                  Vector3 &out) {
    const float flX =
        aflXfm[0][0] * in.x + aflXfm[1][0] * in.y + aflXfm[2][0] * in.z + aflXfm[3][0];
    const float flY =
        aflXfm[0][1] * in.x + aflXfm[1][1] * in.y + aflXfm[2][1] * in.z + aflXfm[3][1];
    const float flZ =
        aflXfm[0][2] * in.x + aflXfm[1][2] * in.y + aflXfm[2][2] * in.z + aflXfm[3][2];
    out.x = flX;
    out.y = flY;
    out.z = flZ;
    out.w = in.w;
}

// Cosine of the turn below which EmitRibbonVerts() mitres a corner rather than keeping the normal.
constexpr float kStraightCos = 0.99985f;

// The camera-space position beside a point on one edge of the ribbon, the near edge taking the
// negated normal. The normal is a screen-space vector, and its y component displaces the depth-free
// camera z.
static inline Vector3 RibbonEdge(const String::Point &point, bool bFarEdge) {
    const float flSign = bFarEdge ? 1.0f : -1.0f;
    return Vector3{point.mCamPos.x + flSign * point.mNormal.x,
                   point.mCamPos.y,
                   point.mCamPos.z + flSign * point.mNormal.y,
                   1.0f};
}

// A cap vertex, the edge position pushed further along the ribbon by cap.
static inline Vector3 CapEdge(const String::Point &point, bool bFarEdge, const Vector2 &cap) {
    Vector3 edge = RibbonEdge(point, bFarEdge);
    edge.x += cap.x;
    edge.z += cap.y;
    return edge;
}

// 0x004b9008
void String::EmitRibbonVerts(Point *pFirst, Point *pLast) {
    Point *pPoint;
    for (pPoint = pFirst; pPoint != pLast; ++pPoint) {
        Vector2 dir;
        SubVec2(&pPoint[1].mScreen.x, &pPoint->mScreen.x, &dir.x);
        NormalizeVec2(&dir.x, &dir.x);
        pPoint->mDir = dir;
        pPoint->mNormal.x = -pPoint->mDir.y;
        pPoint->mNormal.y = pPoint->mDir.x;
        ScaleVec2(&pPoint->mNormal.x, mWidth, &pPoint->mNormal.x);
    }
    pLast->mDir = pLast[-1].mDir;
    pLast->mNormal = pLast[-1].mNormal;

    // Each edge line is a point on the far edge and the segment direction. A turn sharper than
    // mFoldCos folds the ribbon over, and every later normal is negated until the next fold.
    Vector2 line[2];
    AddVec2(&pFirst->mScreen.x, &pFirst->mNormal.x, &line[0].x);
    line[1] = pFirst->mDir;
    int bFolded = 0;
    for (pPoint = pFirst + 1; pPoint != pLast; ++pPoint) {
        const float flTurn =
            pPoint->mDir.x * pPoint[-1].mDir.x + pPoint->mDir.y * pPoint[-1].mDir.y;
        if (flTurn < mFoldCos) {
            bFolded ^= 1;
        }
        if (bFolded != 0) {
            NegateVec2(&pPoint->mNormal.x, &pPoint->mNormal.x);
        }

        const Vector2 previous[] = {line[0], line[1]};
        AddVec2(&pPoint->mScreen.x, &pPoint->mNormal.x, &line[0].x);
        line[1] = pPoint->mDir;
        if (flTurn < kStraightCos) {
            const Vector2 corner = IntersectLines(line, previous);
            SubVec2(&corner.x, &pPoint->mScreen.x, &pPoint->mNormal.x);
        }
    }
    if (bFolded != 0) {
        NegateVec2(&pLast->mNormal.x, &pLast->mNormal.x);
    }

    VertexSlot slot;
    ResolvePointVertexSlot(static_cast<unsigned>(pFirst - &mPoints[0]), slot);
    const Vector2 startCap{-pFirst->mNormal.y, pFirst->mNormal.x};
    if (mHasCaps != 0) {
        (slot.mpVert++)->mPoint = CapEdge(*pFirst, false, startCap);
        (slot.mpVert++)->mPoint = CapEdge(*pFirst, true, startCap);
    }
    for (pPoint = pFirst; pPoint != pLast + 1; ++pPoint) {
        (slot.mpVert++)->mPoint = RibbonEdge(*pPoint, false);
        (slot.mpVert++)->mPoint = RibbonEdge(*pPoint, true);
    }
    if (mHasCaps != 0) {
        const Vector2 endCap = bFolded != 0 ? Vector2{-pLast->mNormal.y, pLast->mNormal.x} :
                                              Vector2{pLast->mNormal.y, -pLast->mNormal.x};
        (slot.mpVert++)->mPoint = CapEdge(*pLast, false, endCap);
        (slot.mpVert++)->mPoint = CapEdge(*pLast, true, endCap);
    }
}

// 0x004b95f8
int String::DrawSelf() {
    Cam *pCam = g_pCurrentCam;
    if (pCam == nullptr || mPoints.size() < kMinRibbonPoints) {
        return 1;
    }

    float aflInverse[kXfmRowCount][kXfmRowFloatCount];
    float aflToCam[kXfmRowCount][kXfmRowFloatCount];
    for (int nRow = 0; nRow < kXfmRowCount; ++nRow) {
        aflInverse[nRow][kXfmRowFloatCount - 1] = 1.0f;
        aflToCam[nRow][kXfmRowFloatCount - 1] = 1.0f;
    }
    sceVu0InversMatrix(aflInverse[0], pCam->mWorldXfm[0]);
    sceVu0Sub005e7ab0(aflToCam[0], aflInverse[0], mWorldXfm[0]);

    const float flNear = pCam->GetNearPlane() + kNearPlaneMargin;
    int bAllClipped = 1;
    for (auto &point : mPoints) {
        TransformPoint(aflToCam, point.mPos, point.mCamPos);
        point.mClipped = point.mCamPos.y < flNear ? 1 : 0;
        bAllClipped &= point.mClipped;
    }
    if (bAllClipped != 0) {
        return 1;
    }

    // A clipped point moves onto the near plane towards an unclipped neighbour, the next one first.
    Point *const pFirst = &mPoints.front();
    Point *const pLast = &mPoints.back();
    for (Point *pPoint = pFirst; pPoint != pFirst + mPoints.size(); ++pPoint) {
        if (pPoint->mClipped == 0) {
            continue;
        }
        const Point *pOther;
        if (pPoint != pLast && pPoint[1].mClipped == 0) {
            pOther = pPoint + 1;
        } else if (pPoint != pFirst && pPoint[-1].mClipped == 0) {
            pOther = pPoint - 1;
        } else {
            continue;
        }
        const float flT = (flNear - pPoint->mCamPos.y) / (pOther->mCamPos.y - pPoint->mCamPos.y);
        const float flS = 1.0f - flT;
        pPoint->mCamPos.x = pOther->mCamPos.x * flT + pPoint->mCamPos.x * flS;
        pPoint->mCamPos.y = pOther->mCamPos.y * flT + pPoint->mCamPos.y * flS;
        pPoint->mCamPos.z = pOther->mCamPos.z * flT + pPoint->mCamPos.z * flS;
        pPoint->mCamPos.w = pOther->mCamPos.w;
        pPoint->mClipped = 0;
    }

    for (auto &point : mPoints) {
        if (point.mClipped == 0) {
            const float flDepth = fabsf(point.mCamPos.y);
            point.mScreen.x = point.mCamPos.x / flDepth;
            point.mScreen.y = point.mCamPos.z / flDepth;
        }
    }

    if (mLinePairs == 0) {
        EmitRibbonVerts(pFirst, pLast);
    } else {
        for (unsigned i = 0; i < mPoints.size() - 1; i += kPointsPerPair) {
            Point *pPairFirst = &mPoints[i];
            Point *pPairEnd = pPairFirst + 1;
            if (pPairFirst->mClipped == 0 && pPairEnd->mClipped == 0) {
                EmitRibbonVerts(pPairFirst, pPairEnd);
                continue;
            }

            // A pair with a clipped point collapses every vertex it governs onto its first point.
            VertexSlot slot;
            ResolvePointVertexSlot(i, slot);
            if (mHasCaps != 0) {
                (slot.mpVert++)->mPoint = pPairFirst->mCamPos;
                (slot.mpVert++)->mPoint = pPairFirst->mCamPos;
            }
            Point *pPoint = pPairFirst;
            for (; pPoint != pPairEnd; ++pPoint) {
                (slot.mpVert++)->mPoint = pPoint->mCamPos;
                (slot.mpVert++)->mPoint = pPoint->mCamPos;
            }
            if (mHasCaps != 0) {
                (slot.mpVert++)->mPoint = pPoint[-1].mCamPos;
                (slot.mpVert++)->mPoint = pPoint[-1].mCamPos;
            }
        }
    }

    mpMesh->SyncChanged(Mesh::kSyncPoints);
    memcpy(mpMesh->mLocalXfm, g_pCurrentCam->mWorldXfm, sizeof(mpMesh->mLocalXfm));
    mpMesh->mDirty = 1;
    mpMesh->UpdateWorldXfm(nullptr, 0);
    mpMesh->Draw();
    return 1;
}

// 0x004b9a68
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

// 0x004b9b30
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

// 0x004bf3c0
int String::GetNumPoints() const {
    return static_cast<int>(mPoints.size());
}

// 0x004bf738
void String::SetPointPos(int nIndex, const Vector3 &pos) {
    mPoints[nIndex].mPos = pos;
}

// 0x004bf400
Vector3 *String::GetPointPos(int nIndex) {
    return &mPoints[nIndex].mPos;
}

// 0x004bf758
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

// 0x004bf418
Color *String::GetPointColor(int nIndex) {
    return &mPoints[nIndex].mColor;
}

// 0x004bf668
void String::SetMat(Mat *pMat) {
    mpMesh->SetMaterial(pMat);
}

// 0x004bf500
Mat *String::GetMat() const {
    return mpMesh->mMat;
}

// 0x004bf3e0
float String::GetWidth() const {
    return mWidth;
}

// 0x004bf708
void String::SetFoldAngle(float flAngle) {
    mFoldAngle = flAngle;
    mFoldCos = cosf(flAngle);
}

// 0x004bf3f8
float String::GetFoldAngle() const {
    return mFoldAngle;
}

// 0x004bf688
void String::SetHasCaps(int nHasCaps) {
    mHasCaps = nHasCaps;
    SetNumPoints(GetNumPoints());
}

// 0x004bf3e8
int String::GetHasCaps() const {
    return mHasCaps;
}

// 0x004bf6c8
void String::SetLinePairs(int nLinePairs) {
    mLinePairs = nLinePairs;
    SetNumPoints(GetNumPoints());
}

// 0x004bf3f0
int String::GetLinePairs() const {
    return mLinePairs;
}

// 0x004bf638
void String::SetHighlight(int nHighlight) {
    mpMesh->SetHighlight(nHighlight);
}

// 0x004bf570
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

// 0x004ba038
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

// 0x004ba258
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

// 0x004bf510
void String::Replace(Object *pFrom, Object *pTo) {
    Drawable::Replace(pFrom, pTo);
    Collideable::Replace(pFrom, pTo);
    Transformable::Replace(pFrom, pTo);
}

// 0x004bf430
const HxStr &String::ClassName() const {
    return g_stringClassName;
}

// 0x004bf858
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

// 0x004ba678
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

// 0x004ba3d0
void String::CreateMesh() {
    mpMesh = g_pfnNewMesh(HxStr("[") + mName + "_mesh]");
    mpMesh->mInternal = 1;
    mpMesh->SetMaterial(mpMat);
    mpMesh->mZMode = Mesh::kZModeZReadOnly;
    mpMesh->mZFunc = Mesh::kZFuncLess;
    mFoldCos = cosf(mFoldAngle);
    SetNumPoints(GetNumPoints());
}

// 0x004bf810
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

// 0x004bed98
void String::Init() {
    g_manager.RegisterClass(g_stringClassName, NewStringObject);
}

} // namespace Rnd
