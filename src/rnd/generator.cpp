#include "rnd/generator.h"

#include <list>
#include <math.h>

#include "math/transform.h"
#include "math/transformops.h"
#include "math/vector3.h"
#include "os/dbg.h"
#include "os/hxstr.h"
#include "os/mem.h"
#include "os/random.h"
#include "rnd/animatable.h"
#include "rnd/cam.h"
#include "rnd/drawable.h"
#include "rnd/manager.h"
#include "rnd/mesh.h"
#include "rnd/multimesh.h"
#include "rnd/object.h"
#include "rnd/particle.h"
#include "rnd/particlesys.h"
#include "rnd/stream.h"
#include "rnd/transanim.h"
#include "rnd/transformable.h"
#include "rnd/view.h"

namespace Rnd {

namespace {

// The only revision Save() writes.
constexpr int kGeneratorRevision = 7;

// Lowest revision Load() refuses.
constexpr int kGeneratorRejectedRevision = 8;

// First revision that stores the four generation bounds and the path variation separately from one
// another. A revision of 0 stores one value for each pair.
constexpr int kGeneratorSplitBoundsRevision = 1;

// First revision whose three base blocks are present.
constexpr int kGeneratorBaseBlockRevision = 2;

// First revision that stores mBirthSquareDist already squared.
constexpr int kGeneratorSquaredDistRevision = 3;

// First revision that stores mView.
constexpr int kGeneratorViewRevision = 4;

// First revision that stores mAnimateFromStart.
constexpr int kGeneratorAnimateFromStartRevision = 5;

// First revision that stores the two path frames.
constexpr int kGeneratorPathFrameRevision = 6;

// First revision that stores mMultiMesh and mParticleSys, and the first that drops the
// child-of-generator flag.
constexpr int kGeneratorSubObjectRevision = 7;

constexpr char kNoObject[] = "no object";
constexpr char kQuotedTextFormat[] = "\"%s\"";
constexpr char kFloatFormat[] = "%.2f";
constexpr char kCountFormat[] = "%u";
constexpr char kIndexFormat[] = "%d";
constexpr char kTrueText[] = "true";
constexpr char kFalseText[] = "false";

// An empty HxStr stores a null buffer, and the binary substitutes the program-wide empty-string
// pointer at 0x006fbd10 rather than passing null to the formatter.
const char *NameText(const Object *pObject) {
    return pObject->mName.mStr != nullptr ? pObject->mName.mStr : "";
}

void PrintObjectRef(Dbg &sink, const Object *pObject) {
    if (pObject == nullptr) {
        sink.Print(kNoObject);
        return;
    }
    sink.Format(kQuotedTextFormat, NameText(pObject));
}

// Each reference is written as the referenced object's name including its terminator. An absent
// reference writes one zero byte, which is the empty name a reader resolves to nothing.
void WriteObjectRef(Stream &stream, const Object *pObject) {
    if (pObject == nullptr) {
        const char chTerminator = '\0';
        stream.Write(&chTerminator, sizeof(chTerminator));
        return;
    }
    stream.Write(NameText(pObject), pObject->mName.mLen + 1);
}

void PrintBool(Dbg &sink, int nValue) {
    sink.Print(nValue != 0 ? kTrueText : kFalseText);
}

// The padding word of a row is not written.
void PrintRow(Dbg &sink, const Vector3 &row) {
    sink.Print("\n\t");
    sink.Print("(x:");
    sink.Format(kFloatFormat, row.x);
    sink.Print(" y:");
    sink.Format(kFloatFormat, row.y);
    sink.Print(" z:");
    sink.Format(kFloatFormat, row.z);
    sink.Print(")");
}

// A byte is written for each of the two flags even though both are stored as words.
void WriteBool(Stream &stream, int nValue) {
    const char chFlag = static_cast<char>(nValue);
    stream.Write(&chFlag, sizeof(chFlag));
}

int ReadBool(Stream &stream) {
    char chFlag = 0;
    stream.Read(&chFlag, sizeof(chFlag));
    return chFlag != 0 ? 1 : 0;
}

// The tag the allocation operators bill to.
// NTSC-U/C: 0x0081c1d8, PAL: 0x0085ecf8
const char *const kGeneratorTag = "Rnd::Generator";

// The next spawn frame the constructor starts from, a hand-written sentinel whose bit pattern
// is 0xcb18967f.
constexpr float kUnsetFrame = -9999999.0f;

constexpr float kDefaultRateGen = 100.0f;

// The reference handling each object member repeats. Every call site in the image open-codes it.
template <typename T>
void ReleaseObjectRef(Object *pOwner, T *pRef) {
    if (pRef != nullptr) {
        pRef->RemoveRef(pOwner);
    }
}

template <typename T>
void AcquireObjectRef(Object *pOwner, T *pRef) {
    if (pRef != nullptr) {
        pRef->AddRef(pOwner);
    }
}

// The shape Rnd::Button::Replace() also has. The assignment is skipped when the member was null,
// which the image does for every member.
template <typename T>
void ReplaceObjectRef(Object *pOwner, T **ppRef, Object *pFrom, Object *pTo) {
    if (*ppRef != pFrom) {
        return;
    }
    if (pFrom != nullptr) {
        pFrom->RemoveRef(pOwner);
    }
    if (*ppRef != nullptr) {
        *ppRef = pTo != nullptr ? dynamic_cast<T *>(pTo) : nullptr;
    }
    AcquireObjectRef(pOwner, *ppRef);
}

void CopyRow(float *pRow, const Vector3 &row) {
    pRow[0] = row.x;
    pRow[1] = row.y;
    pRow[2] = row.z;
    pRow[3] = row.w;
}

// Install a transform as the local transform of a drawn subject and mark it dirty.
template <typename T>
void InstallLocalXfm(T *pTarget, const Transform &xfm) {
    CopyRow(pTarget->mLocalXfm[0], xfm.mBasisX);
    CopyRow(pTarget->mLocalXfm[1], xfm.mBasisY);
    CopyRow(pTarget->mLocalXfm[2], xfm.mBasisZ);
    CopyRow(pTarget->mLocalXfm[3], xfm.mTranslation);
    pTarget->mDirty = 1;
}

// Resolve one serialised reference through the object registry. The reader is the same in all six
// places Load() uses it, and the narrowing cast is what the binary performs.
template <typename T>
T *ReadObjectRef(Stream &stream) {
    HxStr name(nullptr);
    stream.ReadString(name);
    return dynamic_cast<T *>(TheManager.Find(name));
}

// A path bound SetPath() replaces with the matching end of the path's keyframe range.
constexpr float kPathKeyframeBound = -1.0f;

// The rows of a world transform SetFrameSelf() reads from the birth camera.
constexpr int kXfmRowAxisY = 1;
constexpr int kXfmRowTranslation = 3;

// The components of mPathVarMax.
enum PathVarAxis {
    kPathVarAxisX = 0,
    kPathVarAxisY = 1,
    kPathVarAxisZ = 2,
};

// The draw paths of the table in DrawShowing(), in its order.
enum DrawPath {
    kDrawPathView = 0,
    kDrawPathMesh = 1,
    kDrawPathMultiMesh = 2,
    kDrawPathParticle = 3,
};

// The value of pi the image uses, one unit in the last place below the nearest float.
constexpr float kPi = 3.1415925f;
constexpr float kDegreesPerHalfTurn = 180.0f;

// Scale that maps a 31-bit NextRandomValue() result onto the unit range, 2 to the power of -31.
constexpr float kRandomUnitScale = 1.0f / 2147483648.0f;

// A random value interpolated from the high end of a range toward the low end.
inline float RandomInRange(float flLow, float flHigh) {
    return static_cast<float>(NextRandomValue()) * kRandomUnitScale * (flLow - flHigh) + flHigh;
}

// A random angle within flMax degrees either way, in radians, or zero when flMax is not positive.
inline float RandomDegreesToRadians(float flMax) {
    const float flDegrees = 0.0f < flMax ? RandomInRange(-flMax, flMax) : 0.0f;
    return flDegrees * kPi / kDegreesPerHalfTurn;
}

// The identity, with every padding word at 1.0.
inline void SetIdentity(Transform &xfm) {
    xfm.mBasisX.x = 1.0f;
    xfm.mBasisX.y = 0.0f;
    xfm.mBasisX.z = 0.0f;
    xfm.mBasisX.w = 1.0f;
    xfm.mBasisY.x = 0.0f;
    xfm.mBasisY.y = 1.0f;
    xfm.mBasisY.z = 0.0f;
    xfm.mBasisY.w = 1.0f;
    xfm.mBasisZ.x = 0.0f;
    xfm.mBasisZ.y = 0.0f;
    xfm.mBasisZ.z = 1.0f;
    xfm.mBasisZ.w = 1.0f;
    xfm.mTranslation.x = 0.0f;
    xfm.mTranslation.y = 0.0f;
    xfm.mTranslation.z = 0.0f;
    xfm.mTranslation.w = 1.0f;
}

} // namespace

// NTSC-U/C: 0x006e8280, PAL: 0x0072bc08
HxStr g_generatorClassName("Generator");

// NTSC-U/C: 0x0045b3d0, PAL: 0x00498a40
static Dbg &operator<<(Dbg &sink, const Generator::Instance &instance) {
    sink.Print("(frameOrg: ");
    sink.Format(kFloatFormat, instance.mFrameOrg);
    sink.Print(" xfmMod:");
    PrintRow(sink, instance.mXfmMod.mBasisX);
    PrintRow(sink, instance.mXfmMod.mBasisY);
    PrintRow(sink, instance.mXfmMod.mBasisZ);
    PrintRow(sink, instance.mXfmMod.mTranslation);
    sink.Print(")");
    return sink;
}

// NTSC-U/C: 0x0045d6d0, PAL: 0x0049ad68
static Dbg &operator<<(Dbg &sink, const std::list<Generator::Instance> &instances) {
    sink.Print("(size:");
    sink.Format(kCountFormat, instances.size());
    sink.Print(")");

    int nIndex = 0;
    for (std::list<Generator::Instance>::const_iterator it = instances.begin();
         it != instances.end();
         ++it) {
        sink.Print("\n");
        sink.Format(kIndexFormat, nIndex);
        sink.Print("\t");
        sink << *it;
        ++nIndex;
    }
    return sink;
}

Generator::Generator(const HxStr &name)
    : Object(name), mPath(nullptr), mPathStartFrame(0.0f), mPathEndFrame(0.0f), mMesh(nullptr),
      mView(nullptr), mMultiMesh(nullptr), mParticleSys(nullptr), mAnimateFromStart(1),
      mNextSpawnFrame(kUnsetFrame), mBirthFrontOnly(0), mBirthSquareDistCull(0),
      mBirthSquareDist(0.0f), mBirthCam(nullptr), mRateGenLow(kDefaultRateGen),
      mRateGenHigh(kDefaultRateGen), mScaleGenLow(1.0f), mScaleGenHigh(1.0f) {
    mPathVarMax[0] = 0.0f;
    mPathVarMax[1] = 0.0f;
    mPathVarMax[2] = 0.0f;
}

Generator::~Generator() {
    ReleaseRefs();
    ReleaseAllRefs();
}

void Generator::Replace(Object *pFrom, Object *pTo) {
    Transformable::Replace(pFrom, pTo);
    Drawable::Replace(pFrom, pTo);
    Animatable::Replace(pFrom, pTo);
    ReplaceObjectRef(this, &mMesh, pFrom, pTo);
    ReplaceObjectRef(this, &mPath, pFrom, pTo);
    ReplaceObjectRef(this, &mBirthCam, pFrom, pTo);
    ReplaceObjectRef(this, &mView, pFrom, pTo);
    ReplaceObjectRef(this, &mMultiMesh, pFrom, pTo);
    ReplaceObjectRef(this, &mParticleSys, pFrom, pTo);
}

void Generator::Copy(const Object *pSource, unsigned nFlags) {
    // Yes, the result is used without a null test, so a source that is not an emitter is read
    // through null.
    const Generator *pGenerator = dynamic_cast<const Generator *>(pSource);
    Transformable::Copy(pSource, nFlags);
    Drawable::Copy(pSource, nFlags);
    Animatable::Copy(pSource, nFlags);
    ReleaseRefs();
    mMesh = pGenerator->mMesh;
    mPath = pGenerator->mPath;
    mBirthFrontOnly = pGenerator->mBirthFrontOnly;
    mBirthSquareDist = pGenerator->mBirthSquareDist;
    mBirthCam = pGenerator->mBirthCam;
    mRateGenLow = pGenerator->mRateGenLow;
    mRateGenHigh = pGenerator->mRateGenHigh;
    mScaleGenLow = pGenerator->mScaleGenLow;
    mScaleGenHigh = pGenerator->mScaleGenHigh;
    mPathVarMax[0] = pGenerator->mPathVarMax[0];
    mPathVarMax[1] = pGenerator->mPathVarMax[1];
    mPathVarMax[2] = pGenerator->mPathVarMax[2];
    mView = pGenerator->mView;
    mAnimateFromStart = pGenerator->mAnimateFromStart;
    mPathEndFrame = pGenerator->mPathEndFrame;
    mPathStartFrame = pGenerator->mPathStartFrame;
    mMultiMesh = pGenerator->mMultiMesh;
    mParticleSys = pGenerator->mParticleSys;
    AcquireRefs();
}

void *Generator::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, kGeneratorTag);
}

void Generator::operator delete(void *pBlock) {
    OperatorDeleteOverride(pBlock, kGeneratorTag);
}

int Generator::NumInstances() {
    return mInstances.size();
}

void Generator::ReleaseRefs() {
    ReleaseObjectRef(this, mMesh);
    ReleaseObjectRef(this, mPath);
    ReleaseObjectRef(this, mBirthCam);
    ReleaseObjectRef(this, mView);
    ReleaseObjectRef(this, mMultiMesh);
    ReleaseObjectRef(this, mParticleSys);
    mInstances.clear();
}

void Generator::AcquireRefs() {
    AcquireObjectRef(this, mMesh);
    AcquireObjectRef(this, mPath);
    AcquireObjectRef(this, mBirthCam);
    AcquireObjectRef(this, mView);
    AcquireObjectRef(this, mMultiMesh);
    AcquireObjectRef(this, mParticleSys);
    Regenerate();
}

void Generator::SetMesh(Mesh *pMesh) {
    ReleaseObjectRef(this, mMesh);
    mMesh = pMesh;
    AcquireObjectRef(this, mMesh);
    ReleaseObjectRef(this, mView);
    mView = nullptr;
    ReleaseObjectRef(this, mMultiMesh);
    mMultiMesh = nullptr;
    ReleaseObjectRef(this, mParticleSys);
    mParticleSys = nullptr;
}

void Generator::SetView(View *pView) {
    ReleaseObjectRef(this, mMesh);
    mMesh = nullptr;
    ReleaseObjectRef(this, mView);
    mView = pView;
    AcquireObjectRef(this, mView);
    ReleaseObjectRef(this, mMultiMesh);
    mMultiMesh = nullptr;
    ReleaseObjectRef(this, mParticleSys);
    mParticleSys = nullptr;
}

void Generator::SetMultiMesh(MultiMesh *pMultiMesh) {
    ReleaseObjectRef(this, mMesh);
    mMesh = nullptr;
    ReleaseObjectRef(this, mView);
    mView = nullptr;
    ReleaseObjectRef(this, mMultiMesh);
    mMultiMesh = pMultiMesh;
    AcquireObjectRef(this, mMultiMesh);
    ReleaseObjectRef(this, mParticleSys);
    mParticleSys = nullptr;
}

void Generator::SetParticleSys(ParticleSys *pParticleSys) {
    ReleaseObjectRef(this, mMesh);
    mMesh = nullptr;
    ReleaseObjectRef(this, mView);
    mView = nullptr;
    ReleaseObjectRef(this, mMultiMesh);
    mMultiMesh = nullptr;
    ReleaseObjectRef(this, mParticleSys);
    mParticleSys = pParticleSys;
    AcquireObjectRef(this, mParticleSys);
    Regenerate();
}

void Generator::SetBirthCamera(Cam *pCam) {
    ReleaseObjectRef(this, mBirthCam);
    mBirthCam = pCam;
    AcquireObjectRef(this, mBirthCam);
}

void Generator::SetPath(TransAnim *pPath, float flStartFrame, float flEndFrame) {
    ReleaseObjectRef(this, mPath);
    mPath = pPath;
    AcquireObjectRef(this, mPath);
    mPathStartFrame =
        mPath != nullptr && flStartFrame == kPathKeyframeBound ? mPath->StartFrame() : flStartFrame;
    mPathEndFrame = mPath != nullptr && flEndFrame == kPathKeyframeBound ?
                        mPath->FilteredFrameEnd() :
                        flEndFrame;
}

void Generator::SetFrameSelf(float flFrame) {
    if (mNextSpawnFrame == kUnsetFrame) {
        mNextSpawnFrame = flFrame;
        return;
    }

    // Expire the instances whose age has left the path span, keeping the particle walk in step.
    const float flDirection = 0.0f < mPathEndFrame - mPathStartFrame ? 1.0f : -1.0f;
    mParticleCursor = mParticleSys != nullptr ? mParticleSys->GetLiveParticles() : nullptr;
    std::list<Instance>::iterator it = mInstances.begin();
    while (it != mInstances.end()) {
        const float flAge = flFrame - it->mFrameOrg;
        if (flDirection * (mPathEndFrame - mPathStartFrame) < flAge || flAge < 0.0f) {
            if (flAge < 0.0f) {
                mNextSpawnFrame = it->mFrameOrg;
            }
            it = mInstances.erase(it);
            if (mParticleCursor != nullptr) {
                mParticleCursor = mParticleSys->FreeParticle(mParticleCursor);
            }
        } else {
            ++it;
            if (mParticleCursor != nullptr) {
                mParticleCursor = mParticleCursor->mNext;
            }
        }
    }

    if (mView != nullptr && mAnimateFromStart == 0) {
        mView->SetFrame(mFilteredFrame);
    }
    if (mRateGenLow < 0.0f) {
        return;
    }
    const float flEarliest = flFrame - fabsf(mPathEndFrame - mPathStartFrame);
    if (mNextSpawnFrame < flEarliest) {
        mNextSpawnFrame = flEarliest;
    }
    if (flFrame + mRateGenHigh < mNextSpawnFrame) {
        mNextSpawnFrame = flFrame + mRateGenHigh;
    }

    while (mNextSpawnFrame <= flFrame) {
        if (mBirthCam != nullptr) {
            Vector3 offset;
            offset.w = 1.0f;
            Rnd::Subtract(
                mWorldXfm[kXfmRowTranslation], mBirthCam->mWorldXfm[kXfmRowTranslation], &offset.x);
            if (mBirthFrontOnly != 0) {
                const float *pAxis = mBirthCam->mWorldXfm[kXfmRowAxisY];
                if (offset.x * pAxis[0] + offset.y * pAxis[1] + offset.z * pAxis[2] < 0.0f) {
                    return;
                }
            }
            const float flSquareDist =
                offset.x * offset.x + offset.y * offset.y + offset.z * offset.z;
            if (mBirthSquareDistCull != 0 && mBirthSquareDist < flSquareDist) {
                return;
            }
        }

        Instance instance;
        instance.mFrameOrg = mNextSpawnFrame;
        instance.mScale.w = 1.0f;
        SetIdentity(instance.mXfmMod);

        // A random rotation within mPathVarMax degrees about each axis, then the world transform.
        Vector3 angles;
        angles.x = RandomDegreesToRadians(mPathVarMax[kPathVarAxisX]);
        angles.y = RandomDegreesToRadians(mPathVarMax[kPathVarAxisY]);
        angles.z = RandomDegreesToRadians(mPathVarMax[kPathVarAxisZ]);
        angles.w = 1.0f;
        Rnd::MakeRotMatrix(&angles.x, &instance.mXfmMod.mBasisX.x);
        XfmConcat(&instance.mXfmMod.mBasisX.x, mWorldXfm[0], &instance.mXfmMod.mBasisX.x);

        float flScale = mScaleGenLow;
        if (mScaleGenLow < mScaleGenHigh) {
            flScale = RandomInRange(mScaleGenLow, mScaleGenHigh);
        }
        instance.mScale.x = flScale;
        instance.mScale.y = flScale;
        instance.mScale.z = flScale;
        mInstances.push_front(instance);

        if (mParticleSys != nullptr) {
            mParticleCursor = mParticleSys->AllocParticle();
            if (mParticleCursor != nullptr) {
                mParticleSys->RandomizeColorAndSize(mParticleCursor);
            }
        }
        mNextSpawnFrame += RandomInRange(mRateGenLow, mRateGenHigh);
    }
}

int Generator::DrawShowing() {
    // NTSC-U/C: 0x0081c448, PAL: 0x0085ef68
    // the four draw paths in DrawPath order.
    static void (Generator::*const kDrawPaths[])(const Transform &, float) = {
        &Generator::DrawInstanceView,
        &Generator::DrawInstanceMesh,
        &Generator::DrawInstanceMultiMesh,
        &Generator::DrawInstanceParticle,
    };

    if (mPath == nullptr) {
        return 1;
    }
    if (mMesh == nullptr && mView == nullptr && mMultiMesh == nullptr && mParticleSys == nullptr) {
        return 1;
    }
    void (Generator::*pfnDraw)(const Transform &, float);
    if (mView != nullptr) {
        pfnDraw = kDrawPaths[kDrawPathView];
    } else if (mMesh != nullptr) {
        pfnDraw = kDrawPaths[kDrawPathMesh];
    } else if (mMultiMesh != nullptr) {
        std::list<Transform> &transforms = mMultiMesh->GetTransforms();
        if (transforms.size() != mInstances.size()) {
            Transform blank;
            blank.mBasisX.w = 1.0f;
            blank.mBasisY.w = 1.0f;
            blank.mBasisZ.w = 1.0f;
            blank.mTranslation.w = 1.0f;
            transforms.resize(mInstances.size(), blank);
        }
        mMultiMeshCursor = mMultiMesh->GetTransforms().begin();
        pfnDraw = kDrawPaths[kDrawPathMultiMesh];
    } else if (mParticleSys != nullptr) {
        mParticleCursor = mParticleSys->GetLiveParticles();
        pfnDraw = kDrawPaths[kDrawPathParticle];
    }

    const float flDirection = 0.0f < mPathEndFrame - mPathStartFrame ? 1.0f : -1.0f;
    for (Instance &instance : mInstances) {
        const float flAge = mFilteredFrame - instance.mFrameOrg;
        Transform xfm;
        xfm.mBasisX.w = 1.0f;
        xfm.mBasisY.w = 1.0f;
        xfm.mBasisZ.w = 1.0f;
        xfm.mTranslation.w = 1.0f;
        mPath->EvalFrame(flAge * flDirection + mPathStartFrame, &xfm.mBasisX.x, 1);
        Rnd::Scale(&instance.mScale.x, &xfm.mBasisX.x, &xfm.mBasisX.x);
        // Yes, the output is also the first input.
        XfmConcat(&xfm.mBasisX.x, &instance.mXfmMod.mBasisX.x, &xfm.mBasisX.x);
        (this->*pfnDraw)(xfm, flAge);
    }

    if (mMultiMesh != nullptr) {
        mMultiMesh->Draw();
    } else if (mParticleSys != nullptr) {
        mParticleSys->Draw();
    }
    return 1;
}

void Generator::DrawInstanceView(const Transform &xfm, float flAge) {
    InstallLocalXfm(mView, xfm);
    if (mAnimateFromStart != 0) {
        mView->SetFrame(flAge);
    }
    mView->UpdateWorldXfm(nullptr, 0);
    mView->Draw();
}

void Generator::DrawInstanceMesh(const Transform &xfm, [[maybe_unused]] float flAge) {
    InstallLocalXfm(mMesh, xfm);
    mMesh->UpdateWorldXfm(nullptr, 0);
    mMesh->Draw();
}

void Generator::DrawInstanceMultiMesh(const Transform &xfm, [[maybe_unused]] float flAge) {
    *mMultiMeshCursor++ = xfm;
}

void Generator::DrawInstanceParticle(const Transform &xfm, [[maybe_unused]] float flAge) {
    if (mParticleCursor != nullptr) {
        mParticleCursor->mPos = xfm.mTranslation;
        mParticleCursor = mParticleCursor->mNext;
    }
}

const HxStr &Generator::ClassName() const {
    return g_generatorClassName;
}

std::list<Generator::Instance> &Generator::Instances() {
    return mInstances;
}

void Generator::Regenerate() {
    if (mParticleSys == nullptr) {
        return;
    }
    mParticleSys->FreeAllParticles();
    // Only the instance count is read. The walk never visits an instance, and mParticleCursor
    // ends on the last particle the loop allocated.
    for (unsigned i = 0; i < mInstances.size(); ++i) {
        mParticleCursor = mParticleSys->AllocParticle();
        if (mParticleCursor != nullptr) {
            mParticleSys->RandomizeColorAndSize(mParticleCursor);
        }
    }
}

void Generator::DumpText(Dbg &sink) {
    Object::DumpText(sink);
    Transformable::DumpText(sink);
    Drawable::DumpText(sink);
    Animatable::DumpText(sink);

    if (sink.mDumpLevel <= 0) {
        return;
    }

    sink.Print("[Generator]\n");
    sink.Print("path:");
    PrintObjectRef(sink, mPath);
    sink.Print(" mesh:");
    PrintObjectRef(sink, mMesh);
    sink.Print("\n");
    sink.Print(" birthFrontOnly:");
    PrintBool(sink, mBirthFrontOnly);
    sink.Print("birthSquareDist:");
    sink.Format(kFloatFormat, mBirthSquareDist);
    sink.Print(" birthCam:");
    PrintObjectRef(sink, mBirthCam);
    sink.Print("\n");
    sink.Print("rateGenLow:");
    sink.Format(kFloatFormat, mRateGenLow);
    sink.Print(" rateGenHigh:");
    sink.Format(kFloatFormat, mRateGenHigh);
    sink.Print("\n");
    sink.Print("scaleGenLow:");
    sink.Format(kFloatFormat, mScaleGenLow);
    sink.Print(" scaleGenHigh:");
    sink.Format(kFloatFormat, mScaleGenHigh);
    sink.Print("\n");
    sink.Print("pathVarMax:(");
    sink.Format(kFloatFormat, mPathVarMax[0]);
    sink.Print(", ");
    sink.Format(kFloatFormat, mPathVarMax[1]);
    sink.Print(", ");
    sink.Format(kFloatFormat, mPathVarMax[2]);
    sink.Print(")\n");
    sink.Print("view:");
    PrintObjectRef(sink, mView);
    sink.Print(" animateFromStart:");
    PrintBool(sink, mAnimateFromStart);
    sink.Print("\n");
    sink.Print("multiMesh:");
    PrintObjectRef(sink, mMultiMesh);
    sink.Print(" particleSys:");
    PrintObjectRef(sink, mParticleSys);
    sink.Print("\n");

    if (sink.mDumpLevel < 2) {
        return;
    }

    sink.Print("instances:");
    sink << mInstances;
    sink.Print("\n");
    sink.Print("pathEndFrame:");
    sink.Format(kFloatFormat, mPathEndFrame);
    sink.Print(" pathStartFrame:");
    sink.Format(kFloatFormat, mPathStartFrame);
    sink.Print("\n");
}

void Generator::Save(Stream &stream) {
    const int nRevision = kGeneratorRevision;
    stream.WriteLE(&nRevision, sizeof(nRevision));

    Transformable::Save(stream);
    Drawable::Save(stream);
    Animatable::Save(stream);

    WriteObjectRef(stream, mMesh);
    WriteObjectRef(stream, mPath);
    WriteBool(stream, mBirthFrontOnly);
    stream.WriteLE(&mBirthSquareDist, sizeof(mBirthSquareDist));
    WriteObjectRef(stream, mBirthCam);
    stream.WriteLE(&mRateGenLow, sizeof(mRateGenLow));
    stream.WriteLE(&mRateGenHigh, sizeof(mRateGenHigh));
    stream.WriteLE(&mScaleGenLow, sizeof(mScaleGenLow));
    stream.WriteLE(&mScaleGenHigh, sizeof(mScaleGenHigh));
    stream.WriteLE(&mPathVarMax[0], sizeof(mPathVarMax[0]));
    stream.WriteLE(&mPathVarMax[1], sizeof(mPathVarMax[1]));
    stream.WriteLE(&mPathVarMax[2], sizeof(mPathVarMax[2]));
    WriteObjectRef(stream, mView);
    WriteBool(stream, mAnimateFromStart);
    stream.WriteLE(&mPathEndFrame, sizeof(mPathEndFrame));
    stream.WriteLE(&mPathStartFrame, sizeof(mPathStartFrame));
    WriteObjectRef(stream, mMultiMesh);
    WriteObjectRef(stream, mParticleSys);
}

void Generator::Load(Stream &stream) {
    int nRevision = 0;
    stream.ReadLE(&nRevision, sizeof(nRevision));
    if (nRevision >= kGeneratorRejectedRevision) {
        Rnd::TheDbg.Notify("Can't load new Generator\n");
        return;
    }

    if (nRevision >= kGeneratorBaseBlockRevision) {
        Transformable::Load(stream);
        Drawable::Load(stream);
        Animatable::Load(stream);
    }

    if (mMesh != nullptr) {
        mMesh->RemoveRef(this);
    }
    if (mPath != nullptr) {
        mPath->RemoveRef(this);
    }
    if (mBirthCam != nullptr) {
        mBirthCam->RemoveRef(this);
    }
    if (mView != nullptr) {
        mView->RemoveRef(this);
    }
    if (mMultiMesh != nullptr) {
        mMultiMesh->RemoveRef(this);
    }
    if (mParticleSys != nullptr) {
        mParticleSys->RemoveRef(this);
    }
    mInstances.clear();

    mMesh = ReadObjectRef<Mesh>(stream);
    mPath = ReadObjectRef<TransAnim>(stream);

    if (nRevision < kGeneratorSubObjectRevision) {
        // The flag an emitter that spawned another emitter used to store. A cleared flag is the
        // case the build no longer supports, and the report is all that is left of it.
        if (ReadBool(stream) == 0) {
            Rnd::TheDbg.Notify("%s no longer supports childOfGen\n", NameText(this));
        }
    }

    if (nRevision < kGeneratorSplitBoundsRevision) {
        stream.ReadLE(&mRateGenHigh, sizeof(mRateGenHigh));
        stream.ReadLE(&mScaleGenHigh, sizeof(mScaleGenHigh));
    }

    mBirthFrontOnly = ReadBool(stream);
    stream.ReadLE(&mBirthSquareDist, sizeof(mBirthSquareDist));
    mBirthCam = ReadObjectRef<Cam>(stream);

    if (nRevision < kGeneratorSplitBoundsRevision) {
        mRateGenLow = mRateGenHigh;
        mScaleGenLow = mScaleGenHigh;
        mPathVarMax[0] = 0.0f;
        mPathVarMax[2] = 0.0f;
        mPathVarMax[1] = 0.0f;
    } else {
        stream.ReadLE(&mRateGenLow, sizeof(mRateGenLow));
        stream.ReadLE(&mRateGenHigh, sizeof(mRateGenHigh));
        stream.ReadLE(&mScaleGenLow, sizeof(mScaleGenLow));
        stream.ReadLE(&mScaleGenHigh, sizeof(mScaleGenHigh));
        stream.ReadLE(&mPathVarMax[0], sizeof(mPathVarMax[0]));
        stream.ReadLE(&mPathVarMax[1], sizeof(mPathVarMax[1]));
        stream.ReadLE(&mPathVarMax[2], sizeof(mPathVarMax[2]));
    }

    if (nRevision < kGeneratorSquaredDistRevision) {
        mBirthSquareDist = mBirthSquareDist * mBirthSquareDist;
    } else if (nRevision < kGeneratorViewRevision) {
        // A reference and a word the build no longer uses. The resolved object and the word are
        // both discarded.
        HxStr discardedName(nullptr);
        stream.ReadString(discardedName);
        (void)TheManager.Find(discardedName); // Yes, the binary discards this call's result.
        int nDiscarded = 0;
        stream.ReadLE(&nDiscarded, sizeof(nDiscarded));
    }

    if (nRevision >= kGeneratorViewRevision) {
        mView = ReadObjectRef<View>(stream);
    }

    if (nRevision >= kGeneratorAnimateFromStartRevision) {
        mAnimateFromStart = ReadBool(stream);
    }

    if (nRevision < kGeneratorPathFrameRevision) {
        if (mPath != nullptr) {
            mPathEndFrame = mPath->FilteredFrameEnd();
        }
        mPathStartFrame = 0.0f;
    } else {
        stream.ReadLE(&mPathEndFrame, sizeof(mPathEndFrame));
        stream.ReadLE(&mPathStartFrame, sizeof(mPathStartFrame));
    }

    if (nRevision >= kGeneratorSubObjectRevision) {
        mMultiMesh = ReadObjectRef<MultiMesh>(stream);
        mParticleSys = ReadObjectRef<ParticleSys>(stream);
    }

    if (mMesh != nullptr) {
        mMesh->AddRef(this);
    }
    if (mPath != nullptr) {
        mPath->AddRef(this);
    }
    if (mBirthCam != nullptr) {
        mBirthCam->AddRef(this);
    }
    if (mView != nullptr) {
        mView->AddRef(this);
    }
    if (mMultiMesh != nullptr) {
        mMultiMesh->AddRef(this);
    }
    if (mParticleSys != nullptr) {
        mParticleSys->AddRef(this);
    }

    Regenerate();
}

Generator *NewGenerator(const HxStr &name) {
    try {
        return new Generator(name);
    } catch (...) {
        return nullptr; // The binary's handler returns null.
    }
}

// NTSC-U/C: 0x0045e300, PAL: 0x0049b9a8
// The thunk the class registry stores. The null test in the body is the conversion of a Generator
// pointer to its virtual Rnd::Object base rather than a check the source asks for.
static Object *NewGeneratorObject(const HxStr &name) {
    return NewGenerator(name);
}

void Generator::Init() {
    TheManager.RegisterClass(g_generatorClassName, NewGeneratorObject);
}

} // namespace Rnd
