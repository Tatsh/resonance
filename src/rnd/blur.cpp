#include "rnd/blur.h"

#include <algorithm>
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

const char *NameText(const Object *pObject) {
    return pObject->mName.mStr != nullptr ? pObject->mName.mStr : g_szEmptyString;
}

// DumpText() continues on the sink the preceding label returned and discards the result of the
// name print.
void PrintObjectName(FailSink *pSink, const Object *pObject) {
    if (pObject != nullptr) {
        pSink->Format(kQuotedTextFormat, NameText(pObject));
    } else {
        pSink->Print("no object");
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
    try {
        return g_pfnNewBlur(name);
    } catch (...) {
        return nullptr; // The binary's handler returns null.
    }
}

// 0x004c3398
Blur *Blur::NewFromHook(const HxStr &name) {
    try {
        return g_pfnNewBlur(name);
    } catch (...) {
        return nullptr;
    }
}

// 0x004c0e70
Blur::Blur(const HxStr &name)
    : Object(name), mpMesh(nullptr), mpText(nullptr), mLength(0), mRate(1), mFalloff(1.0f),
      mCountdown(mRate) {
    AcquireObjectRefs();
}

// 0x004c0be8
Blur::~Blur() {
    ReleaseObjectRefs();
    ReleaseAllRefs();
}

// 0x004c36b0
void Blur::AcquireObjectRefs() {
    if (mpMesh != nullptr) {
        mpMesh->AddRef(this);
    }
    if (mpText != nullptr) {
        mpText->AddRef(this);
    }
    mXfms.clear();
}

// 0x004c3708
void Blur::ReleaseObjectRefs() {
    if (mpMesh != nullptr) {
        mpMesh->RemoveRef(this);
    }
    if (mpText != nullptr) {
        mpText->RemoveRef(this);
    }
}

// 0x004c3758
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

// 0x004c37b8
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

// 0x004c3818
void Blur::SetLength(int nLength) {
    mLength = std::max(nLength, 0);
    mXfms.clear();
}

// 0x004c3858
void Blur::SetRate(int nRate) {
    mRate = std::max(nRate, 1);
    mXfms.clear();
}

// 0x004c34b0
void Blur::SetFalloff(float flFalloff) {
    mFalloff = flFalloff;
}

// 0x004c3490
Mesh *Blur::GetMesh() const {
    return mpMesh;
}

// 0x004c3498
Text *Blur::GetText() const {
    return mpText;
}

// 0x004c34a0
int Blur::GetLength() const {
    return mLength;
}

// 0x004c34a8
int Blur::GetRate() const {
    return mRate;
}

// 0x004c34b8
float Blur::GetFalloff() const {
    return mFalloff;
}

// 0x004c3480
const HxStr &Blur::ClassName() const {
    return g_blurClassName;
}

// 0x004bfee0
void Blur::DumpText(FailSink &sink) {
    Object::DumpText(sink);
    Drawable::DumpText(sink);

    if (sink.mDumpLevel <= 0) {
        return;
    }

    sink.Print("[Blur]\n");
    FailSink *pLine = sink.Print("mesh:");
    PrintObjectName(pLine, mpMesh);
    pLine->Print(" length:")
        ->Format(kCountFormat, mLength)
        ->Print(" rate:")
        ->Format(kCountFormat, mRate)
        ->Print("\n");
    pLine = sink.Print("falloff:")->Format(kFalloffFormat, mFalloff)->Print(" text:");
    PrintObjectName(pLine, mpText);
    pLine->Print("\n");
}

// 0x004c00b0
void Blur::Save(Stream &stream) {
    const int nRevision = kBlurRevision;
    stream.Write(&nRevision, sizeof(nRevision));

    Drawable::Save(stream);

    WriteObjectName(stream, mpMesh);
    Stream &tail = stream.Write(&mLength, sizeof(mLength))
                       .Write(&mRate, sizeof(mRate))
                       .Write(&mFalloff, sizeof(mFalloff));
    WriteObjectName(tail, mpText);
}

// 0x004c0258
void Blur::Load(Stream &stream) {
    stream.Read(&g_nRndBlurLoadRevision, sizeof(g_nRndBlurLoadRevision));
    if (g_nRndBlurLoadRevision >= kBlurRejectedRevision) {
        g_failSink.Report("Can't load new Blur\n");
        return;
    }

    Drawable::Load(stream);
    ReleaseObjectRefs();

    HxStr meshName;
    stream.ReadString(meshName);
    mpMesh = dynamic_cast<Mesh *>(g_manager.Find(meshName));

    stream.Read(&mLength, sizeof(mLength)).Read(&mRate, sizeof(mRate));
    if (g_nRndBlurLoadRevision >= kBlurFalloffRevision) {
        stream.Read(&mFalloff, sizeof(mFalloff));
    }

    if (g_nRndBlurLoadRevision >= kBlurTextRevision) {
        HxStr textName;
        stream.ReadString(textName);
        mpText = dynamic_cast<Text *>(g_manager.Find(textName));
    }

    AcquireObjectRefs();
}

// 0x004c35e8
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

// 0x004c04c0
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

// 0x004c3570
Blur *Blur::NewBlur(const HxStr &name) {
    return new Blur(name);
}

// 0x004c3418
Blur *Blur::Find(const HxStr &name) {
    return dynamic_cast<Blur *>(g_manager.Find(name));
}

// 0x004c3358
void Blur::Init() {
    g_pfnNewBlur = NewBlur;
    g_manager.RegisterClass(g_blurClassName, NewBlurObject);
}

} // namespace Rnd
