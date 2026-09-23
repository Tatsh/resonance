#include "rnd/blur.h"

#include <list>

#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/drawable.h"
#include "rnd/manager.h"
#include "rnd/mesh.h"
#include "rnd/object.h"
#include "rnd/stream.h"
#include "rnd/text.h"

namespace Rnd {

namespace {

// The only revision this build writes. Load() rejects 3 and above, so the reader accepts one
// revision beyond the highest the image can produce.
constexpr int kBlurRevision = 2;

// Highest revision Load() refuses.
constexpr int kBlurRejectedRevision = 3;

// Revision that added mFalloff.
constexpr int kBlurFalloffRevision = 1;

// Revision that added mpText.
constexpr int kBlurTextRevision = 2;

constexpr char kCountFormat[] = "%d";
constexpr char kFalloffFormat[] = "%.2f";
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

// Each subject is written as its name including the terminator. An absent subject writes one zero
// byte, which is the empty name a reader resolves to nothing.
void WriteObjectName(Stream &stream, const Object *pObject) {
    if (pObject != nullptr) {
        stream.WriteBytes(NameText(pObject), pObject->mName.mLen + 1);
    } else {
        const char cEmpty = 0;
        stream.WriteBytes(&cEmpty, sizeof(cEmpty));
    }
}

} // namespace

// 0x006fd248
HxStr g_blurClassName("Blur");

// 0x00894e28
int g_nRndBlurLoadRevision;

Blur *(*g_pfnNewBlur)(const HxStr &name);

// 0x004c34e0
// The thunk the class registry stores. The null test is the conversion of a Blur
// pointer to its virtual Rnd::Object base rather than a check the source asks for.
static Object *NewBlurObject(const HxStr &name) {
    return g_pfnNewBlur(name);
}

Blur::Blur(const HxStr &name)
    : Object(name), mpMesh(nullptr), mpText(nullptr), mLength(0), mRate(1), mFalloff(1.0f),
      mCountdown(mRate) {
    AcquireObjectRefs();
}

Blur::~Blur() {
    ReleaseObjectRefs();
    ReleaseAllRefs();
}

void Blur::AcquireObjectRefs() {
    if (mpMesh != nullptr) {
        mpMesh->AddRef(this);
    }
    if (mpText != nullptr) {
        mpText->AddRef(this);
    }
    mXfms.clear();
}

void Blur::ReleaseObjectRefs() {
    if (mpMesh != nullptr) {
        mpMesh->RemoveRef(this);
    }
    if (mpText != nullptr) {
        mpText->RemoveRef(this);
    }
}

void Blur::SetMesh(Mesh *pMesh) {
    if (mpMesh != nullptr) {
        mpMesh->RemoveRef(this);
    }
    mpMesh = pMesh;
    if (pMesh != nullptr) {
        pMesh->AddRef(this);
    }
    mXfms.clear();
}

void Blur::SetText(Text *pText) {
    if (mpText != nullptr) {
        mpText->RemoveRef(this);
    }
    mpText = pText;
    if (pText != nullptr) {
        pText->AddRef(this);
    }
    mXfms.clear();
}

void Blur::SetLength(int nLength) {
    mLength = nLength >= 0 ? nLength : 0;
    mXfms.clear();
}

void Blur::SetRate(int nRate) {
    mRate = nRate >= 1 ? nRate : 1;
    mXfms.clear();
}

void Blur::SetFalloff(float flFalloff) {
    mFalloff = flFalloff;
}

Mesh *Blur::GetMesh() const {
    return mpMesh;
}

Text *Blur::GetText() const {
    return mpText;
}

int Blur::GetLength() const {
    return mLength;
}

int Blur::GetRate() const {
    return mRate;
}

float Blur::GetFalloff() const {
    return mFalloff;
}

const HxStr &Blur::ClassName() const {
    return g_blurClassName;
}

void Blur::DumpText(FailSink &sink) {
    Object::DumpText(sink);
    Drawable::DumpText(sink);

    if (sink.mDumpLevel <= 0) {
        return;
    }

    sink.Print("[Blur]\n");
    sink.Print("mesh:");
    PrintObjectName(sink, mpMesh);
    sink.Print(" length:");
    sink.Format(kCountFormat, mLength);
    sink.Print(" rate:");
    sink.Format(kCountFormat, mRate);
    sink.Print("\n");
    sink.Print("falloff:");
    sink.Format(kFalloffFormat, mFalloff);
    sink.Print(" text:");
    PrintObjectName(sink, mpText);
    sink.Print("\n");
}

void Blur::Save(Stream &stream) {
    const int nRevision = kBlurRevision;
    stream.Write(&nRevision, sizeof(nRevision));

    Drawable::Save(stream);

    WriteObjectName(stream, mpMesh);
    stream.Write(&mLength, sizeof(mLength));
    stream.Write(&mRate, sizeof(mRate));
    stream.Write(&mFalloff, sizeof(mFalloff));
    WriteObjectName(stream, mpText);
}

void Blur::Load(Stream &stream) {
    stream.Read(&g_nRndBlurLoadRevision, sizeof(g_nRndBlurLoadRevision));
    if (g_nRndBlurLoadRevision >= kBlurRejectedRevision) {
        g_failSink.Report("Can't load new Blur\n");
        return;
    }

    Drawable::Load(stream);
    ReleaseObjectRefs();

    HxStr meshName(nullptr);
    stream.ReadString(meshName);
    mpMesh = dynamic_cast<Mesh *>(g_manager.Find(meshName));

    stream.Read(&mLength, sizeof(mLength));
    stream.Read(&mRate, sizeof(mRate));
    if (g_nRndBlurLoadRevision >= kBlurFalloffRevision) {
        stream.Read(&mFalloff, sizeof(mFalloff));
    }

    if (g_nRndBlurLoadRevision >= kBlurTextRevision) {
        HxStr textName(nullptr);
        stream.ReadString(textName);
        mpText = dynamic_cast<Text *>(g_manager.Find(textName));
    }

    AcquireObjectRefs();
}

void Blur::Copy(const Object *pSource, unsigned nFlags) {
    const Blur *pSourceBlur = dynamic_cast<const Blur *>(pSource);

    Drawable::Copy(pSource, nFlags);
    ReleaseObjectRefs();

    mpMesh = pSourceBlur->mpMesh;
    mpText = pSourceBlur->mpText;
    mLength = pSourceBlur->mLength;
    mRate = pSourceBlur->mRate;
    mFalloff = pSourceBlur->mFalloff;

    AcquireObjectRefs();
}

void Blur::Replace(Object *pFrom, Object *pTo) {
    Drawable::Replace(pFrom, pTo);

    if (mpMesh == pFrom) {
        if (pFrom != nullptr) {
            pFrom->RemoveRef(this);
        }
        if (mpMesh != nullptr) {
            mpMesh = dynamic_cast<Mesh *>(pTo);
        }
        if (mpMesh != nullptr) {
            mpMesh->AddRef(this);
        }
    }

    if (mpText == pFrom) {
        if (pFrom != nullptr) {
            pFrom->RemoveRef(this);
        }
        if (mpText != nullptr) {
            mpText = dynamic_cast<Text *>(pTo);
        }
        if (mpText != nullptr) {
            mpText->AddRef(this);
        }
    }
}

Blur *Blur::NewBlur(const HxStr &name) {
    return new Blur(name);
}

Blur *Blur::Find(const HxStr &name) {
    return dynamic_cast<Blur *>(g_manager.Find(name));
}

void Blur::Init() {
    g_pfnNewBlur = NewBlur;
    g_manager.RegisterClass(g_blurClassName, NewBlurObject);
}

} // namespace Rnd
