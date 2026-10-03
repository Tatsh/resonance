#include "rnd/environ.h"

#include <algorithm>
#include <list>

#include "math/color.h"
#include "os/dbg.h"
#include "os/hxstr.h"
#include "os/mem.h"
#include "rnd/drawable.h"
#include "rnd/light.h"
#include "rnd/manager.h"
#include "rnd/object.h"
#include "rnd/stream.h"

namespace Rnd {

namespace {

// The only revision this build writes, and the highest Load() accepts.
constexpr int kEnvironRevision = 0;

// Bit of the Copy() flags word that suppresses the light list. The sense is inverted with respect
// to kCopyChildLists, which is what the binary does.
constexpr unsigned kCopyNoLights = 0x1;

constexpr char kAlreadyInFormat[] = "%s already in %s\n";

// The allocation tag every environment block is billed to.
constexpr char kEnvironTag[] = "Rnd::Environ";
constexpr char kCountFormat[] = "%d";
constexpr char kSizeFormat[] = "%u";
constexpr char kQuotedTextFormat[] = "\"%s\"";

// An empty HxStr stores a null buffer, and the binary substitutes the program-wide empty-string
// pointer at 0x006fbd10 rather than passing null to the formatter.
const char *NameText(const Object *pObject) {
    return pObject->mName.mStr != nullptr ? pObject->mName.mStr : "";
}

// DumpText() expands this for both colours, one Print and Format pair per component.
inline void PrintColor(Dbg &sink, const Color &color) {
    sink.Print("(r:");
    sink.Format("%.2f", color.r);
    sink.Print(" g:");
    sink.Format("%.2f", color.g);
    sink.Print(" b:");
    sink.Format("%.2f", color.b);
    sink.Print(" a:");
    sink.Format("%.2f", color.a);
    sink.Print(")");
}

} // namespace

// NTSC-U/C: 0x00718d18, PAL: 0x0075cc08
HxStr Environ::sClassName("Environ");

Environ *Environ::sCurrent;

Environ *(*g_pfnNewEnviron)(const HxStr &name);

// NTSC-U/C: 0x00515890, PAL: 0x00555bc0
Environ::Environ(const HxStr &name)
    : Object(name), mAmbient{0.0f, 0.0f, 0.0f, 0.0f}, mFogStart(0.0f), mFogEnd(1.0f),
      mFogDensity(1.0f), mFogColor{1.0f, 1.0f, 1.0f, 1.0f}, mFogMode(kFogModeNone) {
}

// NTSC-U/C: 0x00518fe0, PAL: 0x00559378
Environ::~Environ() {
    if (Environ::sCurrent == this) {
        Environ::sCurrent = nullptr;
    }
    ReleaseLightsRefs();
    ReleaseAllRefs();
}

// NTSC-U/C: 0x00519258, PAL: 0x005595f0
const HxStr &Environ::ClassName() const {
    return Environ::sClassName;
}

// NTSC-U/C: 0x00519470, PAL: 0x00559808
//
// A mode outside the enumeration produces no text at all rather than a fallback name.
Dbg &operator<<(Dbg &sink, FogMode nMode) {
    if (nMode == kFogModeNone) {
        sink.Print("None");
    } else if (nMode == kFogModeVertExp) {
        sink.Print("VertExp");
    } else if (nMode == kFogModeVertExp2) {
        sink.Print("VertExp2");
    } else if (nMode == kFogModeVertLinear) {
        sink.Print("VertLinear");
    } else if (nMode == kFogModePixelExp) {
        sink.Print("PixelExp");
    } else if (nMode == kFogModePixelExp2) {
        sink.Print("PixelExp2");
    } else if (nMode == kFogModePixelLinear) {
        sink.Print("PixelLinear");
    }
    return sink;
}

// NTSC-U/C: 0x005185b0, PAL: 0x00558908
Dbg &operator<<(Dbg &sink, const std::list<Light *> &lights) {
    sink.Print("(size:");
    sink.Format(kSizeFormat, lights.size());
    sink.Print(")");

    int nIndex = 0;
    for (std::list<Light *>::const_iterator it = lights.begin(); it != lights.end(); ++it) {
        sink.Print("\n");
        sink.Format(kCountFormat, nIndex);
        sink.Print("\t");
        const Object *pObject = *it;
        if (pObject != nullptr) {
            sink.Format(kQuotedTextFormat, NameText(pObject));
        } else {
            sink.Print("no object");
        }
        ++nIndex;
    }
    return sink;
}

// NTSC-U/C: 0x00518718, PAL: 0x00558a70
//
// Each entry is written as the referenced object's name including its terminator. An empty entry
// writes one zero byte.
static Stream &operator<<(Stream &stream, const std::list<Light *> &lights) {
    int nCount = lights.size();
    stream.WriteLE(&nCount, sizeof(nCount));

    for (std::list<Light *>::const_iterator it = lights.begin(); it != lights.end(); ++it) {
        const Object *pObject = *it;
        if (pObject != nullptr) {
            stream.Write(NameText(pObject), pObject->mName.mLen + 1);
        } else {
            const char cEmpty = 0;
            stream.Write(&cEmpty, sizeof(cEmpty));
        }
    }
    return stream;
}

// NTSC-U/C: 0x005189f0, PAL: 0x00558d48
static Stream &operator>>(Stream &stream, std::list<Light *> &lights) {
    int nCount = 0;
    stream.ReadLE(&nCount, sizeof(nCount));
    lights.resize(nCount, nullptr);

    for (std::list<Light *>::iterator it = lights.begin(); it != lights.end(); ++it) {
        HxStr name(nullptr);
        stream.ReadString(name);
        *it = dynamic_cast<Light *>(TheManager.Find(name));
    }
    return stream;
}

// NTSC-U/C: 0x00515ce0, PAL: 0x00556010
void Environ::DumpText(Dbg &sink) {
    Object::DumpText(sink);
    Drawable::DumpText(sink);
    if (sink.mDumpLevel <= 0) {
        return;
    }

    sink.Print("[Environ]\n");
    sink.Print("lights:");
    sink << mLights;
    sink.Print("\n");
    sink.Print("ambient:");
    PrintColor(sink, mAmbient);
    sink.Print(" fogStart:");
    sink.Format("%.2f", mFogStart);
    sink.Print("\n");
    sink.Print("fogEnd:");
    sink.Format("%.2f", mFogEnd);
    sink.Print(" fogDensity:");
    sink.Format("%.2f", mFogDensity);
    sink.Print("\n");
    sink.Print("fogColor:");
    PrintColor(sink, mFogColor);
    sink.Print(" fogMode:");
    sink << mFogMode;
    sink.Print("\n");
}

// NTSC-U/C: 0x00518e60, PAL: 0x005591f8
int Environ::DrawShowing() {
    Environ::sCurrent = this;
    return 1;
}

// NTSC-U/C: 0x00519400, PAL: 0x00559798
void Environ::AcquireLightsRefs() {
    for (std::list<Light *>::iterator it = mLights.begin(); it != mLights.end(); ++it) {
        Object *pObject = *it;
        if (pObject != nullptr) {
            pObject->AddRef(this);
        }
    }
}

// NTSC-U/C: 0x00519390, PAL: 0x00559728
void Environ::ReleaseLightsRefs() {
    for (std::list<Light *>::iterator it = mLights.begin(); it != mLights.end(); ++it) {
        Object *pObject = *it;
        if (pObject != nullptr) {
            pObject->RemoveRef(this);
        }
    }
}

// NTSC-U/C: 0x00516028, PAL: 0x00556358
void Environ::Save(Stream &stream) {
    const int nRevision = kEnvironRevision;
    stream.WriteLE(&nRevision, sizeof(nRevision));

    Drawable::Save(stream);
    stream << mLights;

    stream.WriteLE(&mAmbient.r, sizeof(mAmbient.r));
    stream.WriteLE(&mAmbient.g, sizeof(mAmbient.g));
    stream.WriteLE(&mAmbient.b, sizeof(mAmbient.b));
    stream.WriteLE(&mAmbient.a, sizeof(mAmbient.a));

    stream.WriteLE(&mFogStart, sizeof(mFogStart));
    stream.WriteLE(&mFogEnd, sizeof(mFogEnd));
    stream.WriteLE(&mFogDensity, sizeof(mFogDensity));

    stream.WriteLE(&mFogColor.r, sizeof(mFogColor.r));
    stream.WriteLE(&mFogColor.g, sizeof(mFogColor.g));
    stream.WriteLE(&mFogColor.b, sizeof(mFogColor.b));
    stream.WriteLE(&mFogColor.a, sizeof(mFogColor.a));

    stream.WriteLE(&mFogMode, sizeof(mFogMode));
}

// NTSC-U/C: 0x00516260, PAL: 0x00556590
void Environ::Load(Stream &stream) {
    int nRevision = 0;
    stream.ReadLE(&nRevision, sizeof(nRevision));
    if (nRevision > kEnvironRevision) {
        Rnd::TheDbg.Notify("Can't load new Environ\n");
        return;
    }

    Drawable::Load(stream);
    ReleaseLightsRefs();

    stream >> mLights;

    stream.ReadLE(&mAmbient.r, sizeof(mAmbient.r));
    stream.ReadLE(&mAmbient.g, sizeof(mAmbient.g));
    stream.ReadLE(&mAmbient.b, sizeof(mAmbient.b));
    stream.ReadLE(&mAmbient.a, sizeof(mAmbient.a));

    stream.ReadLE(&mFogStart, sizeof(mFogStart));
    stream.ReadLE(&mFogEnd, sizeof(mFogEnd));
    stream.ReadLE(&mFogDensity, sizeof(mFogDensity));

    stream.ReadLE(&mFogColor.r, sizeof(mFogColor.r));
    stream.ReadLE(&mFogColor.g, sizeof(mFogColor.g));
    stream.ReadLE(&mFogColor.b, sizeof(mFogColor.b));
    stream.ReadLE(&mFogColor.a, sizeof(mFogColor.a));

    int nFogMode = 0;
    stream.ReadLE(&nFogMode, sizeof(nFogMode));
    mFogMode = static_cast<FogMode>(nFogMode);

    AcquireLightsRefs();
}

// NTSC-U/C: 0x00516560, PAL: 0x00556890
void Environ::Copy(const Object *pSource, unsigned nFlags) {
    const Environ *pSourceEnviron = dynamic_cast<const Environ *>(pSource);

    Drawable::Copy(pSource, nFlags);
    ReleaseLightsRefs();

    if ((nFlags & kCopyNoLights) == 0) {
        mLights = pSourceEnviron->mLights;
    }

    mAmbient = pSourceEnviron->mAmbient;
    mFogStart = pSourceEnviron->mFogStart;
    mFogEnd = pSourceEnviron->mFogEnd;
    mFogDensity = pSourceEnviron->mFogDensity;
    mFogColor = pSourceEnviron->mFogColor;
    mFogMode = pSourceEnviron->mFogMode;

    AcquireLightsRefs();
}

// NTSC-U/C: 0x005156b0, PAL: 0x005559e0
void Environ::Replace(Object *pFrom, Object *pTo) {
    Drawable::Replace(pFrom, pTo);

    for (std::list<Light *>::iterator it = mLights.begin(); it != mLights.end();) {
        if (*it == pTo) {
            Rnd::TheDbg.Notify(kAlreadyInFormat, NameText(pTo), NameText(this));
        }

        if (*it == pFrom) {
            if (pFrom != nullptr) {
                pFrom->RemoveRef(this);
            }
            if (*it != nullptr) {
                *it = dynamic_cast<Light *>(pTo);
            }
            if (*it != nullptr) {
                (*it)->AddRef(this);
            }
        }

        if (*it == nullptr) {
            it = mLights.erase(it);
        } else {
            ++it;
        }
    }
}

// NTSC-U/C: 0x00519308, PAL: 0x005596a0
Environ *Environ::NewEnviron(const HxStr &name) {
    return new Environ(name);
}

// NTSC-U/C: 0x00518df0, PAL: 0x00559188
void *Environ::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, kEnvironTag);
}

// NTSC-U/C: 0x00518e10, PAL: 0x005591a8
void Environ::operator delete(void *pBlock) {
    OperatorDeleteOverride(pBlock, kEnvironTag);
}

// NTSC-U/C: 0x005166d0, PAL: 0x00556a00
void Environ::AddLight(Light *pLight) {
    if (std::find(mLights.begin(), mLights.end(), pLight) != mLights.end()) {
        Rnd::TheDbg.Notify(kAlreadyInFormat, NameText(pLight), NameText(this));
        return;
    }
    if (pLight != nullptr) {
        pLight->AddRef(this);
    }
    mLights.push_back(pLight);
}

// NTSC-U/C: 0x00516850, PAL: 0x00556b80
void Environ::RemoveLight(Light *pLight) {
    const auto it = std::find(mLights.begin(), mLights.end(), pLight);
    if (it == mLights.end()) {
        return;
    }
    if (pLight != nullptr) {
        pLight->RemoveRef(this);
    }
    mLights.erase(it);
}

// NTSC-U/C: 0x00518eb0, PAL: 0x00559248
Environ *NewEnvironThroughHook(const HxStr &name) {
    try {
        return g_pfnNewEnviron(name);
    } catch (...) {
        return nullptr; // The binary's handler returns null.
    }
}

// NTSC-U/C: 0x00519278, PAL: 0x00559610
Object *CreateRegisteredEnviron(const HxStr &name) {
    try {
        return g_pfnNewEnviron(name);
    } catch (...) {
        return nullptr;
    }
}

// NTSC-U/C: 0x00519568, PAL: 0x00559900
void Environ::RemoveAllLights() {
    ReleaseLightsRefs();
    mLights.clear();
}

} // namespace Rnd
