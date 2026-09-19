#include "rnd/environ.h"

#include <list>

#include "math/color.h"
#include "os/failsink.h"
#include "os/hxstr.h"
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
constexpr char kCountFormat[] = "%d";
constexpr char kSizeFormat[] = "%u";
constexpr char kQuotedTextFormat[] = "\"%s\"";

// An empty HxStr stores a null buffer, and the binary substitutes the program-wide empty-string
// pointer at 0x006fbd10 rather than passing null to the formatter.
const char *NameText(const Object *pObject) {
    return pObject->mName.mStr != nullptr ? pObject->mName.mStr : "";
}

} // namespace

Environ *g_pCurrentEnviron;

Environ *(*g_pfnNewEnviron)(const HxStr &name);

// 0x00519470
//
// A mode outside the enumeration produces no text at all rather than a fallback name.
FailSink &operator<<(FailSink &sink, FogMode nMode) {
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

// 0x005185b0
FailSink &operator<<(FailSink &sink, const std::list<Light *> &lights) {
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

// 0x00518718
//
// Each entry is written as the referenced object's name including its terminator. An empty entry
// writes one zero byte.
static Stream &operator<<(Stream &stream, const std::list<Light *> &lights) {
    int nCount = lights.size();
    stream.Write(&nCount, sizeof(nCount));

    for (std::list<Light *>::const_iterator it = lights.begin(); it != lights.end(); ++it) {
        const Object *pObject = *it;
        if (pObject != nullptr) {
            stream.WriteBytes(NameText(pObject), pObject->mName.mLen + 1);
        } else {
            const char cEmpty = 0;
            stream.WriteBytes(&cEmpty, sizeof(cEmpty));
        }
    }
    return stream;
}

// 0x005189f0
static Stream &operator>>(Stream &stream, std::list<Light *> &lights) {
    int nCount = 0;
    stream.Read(&nCount, sizeof(nCount));
    lights.resize(nCount, nullptr);

    for (std::list<Light *>::iterator it = lights.begin(); it != lights.end(); ++it) {
        HxStr name(nullptr);
        stream.ReadString(name);
        *it = dynamic_cast<Light *>(g_manager.Find(name));
    }
    return stream;
}

int Environ::DrawSelf() {
    g_pCurrentEnviron = this;
    return 1;
}

void Environ::AcquireLightsRefs() {
    for (std::list<Light *>::iterator it = mLights.begin(); it != mLights.end(); ++it) {
        Object *pObject = *it;
        if (pObject != nullptr) {
            pObject->AddRef(this);
        }
    }
}

void Environ::ReleaseLightsRefs() {
    for (std::list<Light *>::iterator it = mLights.begin(); it != mLights.end(); ++it) {
        Object *pObject = *it;
        if (pObject != nullptr) {
            pObject->RemoveRef(this);
        }
    }
}

void Environ::Save(Stream &stream) {
    const int nRevision = kEnvironRevision;
    stream.Write(&nRevision, sizeof(nRevision));

    Drawable::Save(stream);
    stream << mLights;

    stream.Write(&mAmbient.r, sizeof(mAmbient.r));
    stream.Write(&mAmbient.g, sizeof(mAmbient.g));
    stream.Write(&mAmbient.b, sizeof(mAmbient.b));
    stream.Write(&mAmbient.a, sizeof(mAmbient.a));

    stream.Write(&mFogStart, sizeof(mFogStart));
    stream.Write(&mFogEnd, sizeof(mFogEnd));
    stream.Write(&mFogDensity, sizeof(mFogDensity));

    stream.Write(&mFogColor.r, sizeof(mFogColor.r));
    stream.Write(&mFogColor.g, sizeof(mFogColor.g));
    stream.Write(&mFogColor.b, sizeof(mFogColor.b));
    stream.Write(&mFogColor.a, sizeof(mFogColor.a));

    stream.Write(&mFogMode, sizeof(mFogMode));
}

void Environ::Load(Stream &stream) {
    int nRevision = 0;
    stream.Read(&nRevision, sizeof(nRevision));
    if (nRevision > kEnvironRevision) {
        g_failSink.Report("Can't load new Environ\n");
        return;
    }

    Drawable::Load(stream);
    ReleaseLightsRefs();

    stream >> mLights;

    stream.Read(&mAmbient.r, sizeof(mAmbient.r));
    stream.Read(&mAmbient.g, sizeof(mAmbient.g));
    stream.Read(&mAmbient.b, sizeof(mAmbient.b));
    stream.Read(&mAmbient.a, sizeof(mAmbient.a));

    stream.Read(&mFogStart, sizeof(mFogStart));
    stream.Read(&mFogEnd, sizeof(mFogEnd));
    stream.Read(&mFogDensity, sizeof(mFogDensity));

    stream.Read(&mFogColor.r, sizeof(mFogColor.r));
    stream.Read(&mFogColor.g, sizeof(mFogColor.g));
    stream.Read(&mFogColor.b, sizeof(mFogColor.b));
    stream.Read(&mFogColor.a, sizeof(mFogColor.a));

    int nFogMode = 0;
    stream.Read(&nFogMode, sizeof(nFogMode));
    mFogMode = static_cast<FogMode>(nFogMode);

    AcquireLightsRefs();
}

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

void Environ::Replace(Object *pFrom, Object *pTo) {
    Drawable::Replace(pFrom, pTo);

    for (std::list<Light *>::iterator it = mLights.begin(); it != mLights.end();) {
        if (*it == pTo) {
            g_failSink.Report(kAlreadyInFormat, NameText(pTo), NameText(this));
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

Environ *Environ::NewEnviron(const HxStr &name) {
    return new Environ(name);
}

} // namespace Rnd
