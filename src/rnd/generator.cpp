#include "rnd/generator.h"

#include <list>

#include "math/transform.h"
#include "math/vector3.h"
#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/cam.h"
#include "rnd/drawable.h"
#include "rnd/manager.h"
#include "rnd/mesh.h"
#include "rnd/multimesh.h"
#include "rnd/object.h"
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

void PrintObjectRef(FailSink &sink, const Object *pObject) {
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
        stream.WriteBytes(&chTerminator, sizeof(chTerminator));
        return;
    }
    stream.WriteBytes(NameText(pObject), pObject->mName.mLen + 1);
}

void PrintBool(FailSink &sink, int nValue) {
    sink.Print(nValue != 0 ? kTrueText : kFalseText);
}

// The padding word of a row is not written.
void PrintRow(FailSink &sink, const Vector3 &row) {
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
    stream.ReadBytes(&chFlag, sizeof(chFlag));
    return chFlag != 0 ? 1 : 0;
}

// Resolve one serialised reference through the object registry. The reader is the same in all six
// places Load() uses it, and the narrowing cast is what the binary performs.
template <typename T>
T *ReadObjectRef(Stream &stream) {
    HxStr name(nullptr);
    stream.ReadString(name);
    return dynamic_cast<T *>(g_manager.Find(name));
}

} // namespace

// 0x006e8280
HxStr g_generatorClassName("Generator");

// 0x0045b3d0
static FailSink &operator<<(FailSink &sink, const Generator::Instance &instance) {
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

// 0x0045d6d0
static FailSink &operator<<(FailSink &sink, const std::list<Generator::Instance> &instances) {
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

// 0x0045e2e8
const HxStr &Generator::ClassName() const {
    return g_generatorClassName;
}

// 0x0045e2f8
std::list<Generator::Instance> &Generator::Instances() {
    return mInstances;
}

// 0x00459618
void Generator::DumpText(FailSink &sink) {
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

// 0x00459bd8
//
// The instance list is not written, so a reloaded emitter starts empty and Load() repopulates it
// through Regenerate().
void Generator::Save(Stream &stream) {
    const int nRevision = kGeneratorRevision;
    stream.Write(&nRevision, sizeof(nRevision));

    Transformable::Save(stream);
    Drawable::Save(stream);
    Animatable::Save(stream);

    WriteObjectRef(stream, mMesh);
    WriteObjectRef(stream, mPath);
    WriteBool(stream, mBirthFrontOnly);
    stream.Write(&mBirthSquareDist, sizeof(mBirthSquareDist));
    WriteObjectRef(stream, mBirthCam);
    stream.Write(&mRateGenLow, sizeof(mRateGenLow));
    stream.Write(&mRateGenHigh, sizeof(mRateGenHigh));
    stream.Write(&mScaleGenLow, sizeof(mScaleGenLow));
    stream.Write(&mScaleGenHigh, sizeof(mScaleGenHigh));
    stream.Write(&mPathVarMax[0], sizeof(mPathVarMax[0]));
    stream.Write(&mPathVarMax[1], sizeof(mPathVarMax[1]));
    stream.Write(&mPathVarMax[2], sizeof(mPathVarMax[2]));
    WriteObjectRef(stream, mView);
    WriteBool(stream, mAnimateFromStart);
    stream.Write(&mPathEndFrame, sizeof(mPathEndFrame));
    stream.Write(&mPathStartFrame, sizeof(mPathStartFrame));
    WriteObjectRef(stream, mMultiMesh);
    WriteObjectRef(stream, mParticleSys);
}

// 0x0045a090
void Generator::Load(Stream &stream) {
    int nRevision = 0;
    stream.Read(&nRevision, sizeof(nRevision));
    if (nRevision >= kGeneratorRejectedRevision) {
        g_failSink.Report("Can't load new Generator\n");
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
            g_failSink.Report("%s no longer supports childOfGen\n", NameText(this));
        }
    }

    if (nRevision < kGeneratorSplitBoundsRevision) {
        stream.Read(&mRateGenHigh, sizeof(mRateGenHigh));
        stream.Read(&mScaleGenHigh, sizeof(mScaleGenHigh));
    }

    mBirthFrontOnly = ReadBool(stream);
    stream.Read(&mBirthSquareDist, sizeof(mBirthSquareDist));
    mBirthCam = ReadObjectRef<Cam>(stream);

    if (nRevision < kGeneratorSplitBoundsRevision) {
        mRateGenLow = mRateGenHigh;
        mScaleGenLow = mScaleGenHigh;
        mPathVarMax[0] = 0.0f;
        mPathVarMax[2] = 0.0f;
        mPathVarMax[1] = 0.0f;
    } else {
        stream.Read(&mRateGenLow, sizeof(mRateGenLow));
        stream.Read(&mRateGenHigh, sizeof(mRateGenHigh));
        stream.Read(&mScaleGenLow, sizeof(mScaleGenLow));
        stream.Read(&mScaleGenHigh, sizeof(mScaleGenHigh));
        stream.Read(&mPathVarMax[0], sizeof(mPathVarMax[0]));
        stream.Read(&mPathVarMax[1], sizeof(mPathVarMax[1]));
        stream.Read(&mPathVarMax[2], sizeof(mPathVarMax[2]));
    }

    if (nRevision < kGeneratorSquaredDistRevision) {
        mBirthSquareDist = mBirthSquareDist * mBirthSquareDist;
    } else if (nRevision < kGeneratorViewRevision) {
        // A reference and a word the build no longer uses. The resolved object and the word are
        // both discarded.
        HxStr discardedName(nullptr);
        stream.ReadString(discardedName);
        (void)g_manager.Find(discardedName); // Yes, the binary discards this call's result.
        int nDiscarded = 0;
        stream.Read(&nDiscarded, sizeof(nDiscarded));
    }

    if (nRevision >= kGeneratorViewRevision) {
        mView = ReadObjectRef<View>(stream);
    }

    if (nRevision > kGeneratorAnimateFromStartRevision) {
        mAnimateFromStart = ReadBool(stream);
    }

    if (nRevision < kGeneratorPathFrameRevision) {
        if (mPath != nullptr) {
            mPathEndFrame = mPath->EndFrame();
        }
        mPathStartFrame = 0.0f;
    } else {
        stream.Read(&mPathEndFrame, sizeof(mPathEndFrame));
        stream.Read(&mPathStartFrame, sizeof(mPathStartFrame));
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

// 0x0045dcf0
//
// Nothing in the image references this copy. The allocation is billed to the tag "Rnd::Generator"
// and takes 0x160 bytes.
Generator *NewGenerator(const HxStr &name) {
    return new Generator(name);
}

// 0x0045e300. The thunk the class registry stores. The null test in the body is the conversion of
// a Generator pointer to its virtual Rnd::Object base rather than a check the source asks for.
static Object *NewGeneratorObject(const HxStr &name) {
    return NewGenerator(name);
}

// 0x0045dcc0
void Generator::Init() {
    g_manager.RegisterClass(g_generatorClassName, NewGeneratorObject);
}

} // namespace Rnd
