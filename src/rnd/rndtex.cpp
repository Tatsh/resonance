#include <ctype.h>
#include <list>
#include <string.h>
#include <vector>

#include "os/async.h"
#include "os/failsink.h"
#include "os/hxstr.h"
#include "os/mem.h"
#include "rnd/filepath.h"
#include "rnd/stream.h"
#include "rnd/tex.h"
#include "rndartt/apalette.h"

namespace Rnd {

namespace {

// The tag every release in this file bills to, and the file name its asserts record.
constexpr char kTexFileName[] = "rndtex.cpp";

// Line 610 of rndtex.cpp, which FreeLoadedBitmaps() passes to the tagged release.
constexpr int kFreeBitmapLine = 0x262;

// Characters and components Rnd::FilePath splits and rebuilds paths with.
constexpr char kPathSeparator = '/';
constexpr char kBackslash = '\\';
constexpr char kFullStop = '.';
constexpr char kPathSeparators[] = "/";
constexpr char kParentDirectory[] = "..";

// The text of a string, or the shared empty string when the buffer is null.
inline const char *TextOf(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

} // namespace

// 0x004e3dc8
Tex::Tex(const HxStr &name)
    : Object(name), mWidth(0), mHeight(0), mBitsPerPixel(0), mUnknown28(0), mPendingMipMask(0),
      mMipSelect(-0x80), mBitmapPath(nullptr), mGsHandle(-1) {
}

// 0x004e7878
bool Tex::IsLoadComplete() {
    return mPendingMipMask == 0;
}

// 0x004e4410
bool Tex::PollAsyncMips() {
    if (mPendingMipMask == 0) {
        return true;
    }

    for (unsigned nMip = 0; nMip < mMipHandles.size(); ++nMip) {
        if (((mPendingMipMask >> nMip) & 1) == 0) {
            continue;
        }

        void *pBuffer = nullptr;
        const int nStatus = AsyncPollComplete(mMipHandles[nMip], &pBuffer, nullptr);
        if (nStatus == 0) {
            mLoadedBitmaps[nMip] = static_cast<ABitmap *>(pBuffer);
            OnMipLoaded(nMip);
            mPendingMipMask &= ~(1 << nMip);
            continue;
        }
        if (nStatus > 0) {
            g_failSink.Report(
                "Texture %s mip %d: async read error %d\n", mBitmapPath.mStr, nMip, nStatus);
            // A failed read clears its bit and reports the load complete, which stops the caller
            // spinning on a mip that will never arrive.
            mPendingMipMask &= ~(1 << nMip);
            return true;
        }
    }

    if (mPendingMipMask != 0) {
        return false;
    }
    RestoreSurfaces();
    return true;
}

// 0x004e73c8
void Tex::ReloadBitmaps() {
    FreeLoadedBitmaps();
    AllocateBitmapFromStream();
}

// 0x004e7aa8
void Tex::FreeLoadedBitmaps() {
    CancelPendingMips();
    for (const auto pBitmap : mLoadedBitmaps) {
        // A bitmap already resident in GS memory belongs to its slot, so only a copy that never
        // arrived there is released here.
        if (pBitmap != nullptr && mGsHandle == -1) {
            MemFreeTagged(pBitmap, kTexFileName, kFreeBitmapLine);
        }
    }
    mLoadedBitmaps.clear();
}

// 0x007033b8
FilePath FilePath::sRoot("");

// 0x004e4bd8
void FilePath::SetFromRoot(const HxStr &name) {
    if (name.mLen == 0) {
        Clear();
        return;
    }
    const HxStr prefix(HxStr(sRoot) += kPathSeparator);
    const HxStr full = prefix + name;
    HxStr::operator=(full);
    Normalize();
}

// 0x004e7b70
void FilePath::Set(const HxStr &path) {
    HxStr::operator=(path);
    Normalize();
}

// 0x004e4d88
void FilePath::Normalize() {
    HxStr text(TextOf(*this));
    for (char *pch = text.mStr; pch != text.mStr + text.mLen; ++pch) {
        if (isupper(*pch)) {
            *pch = tolower(*pch);
        }
    }
    for (char *pch = text.mStr; pch != text.mStr + text.mLen; ++pch) {
        if (*pch == kBackslash) {
            *pch = kPathSeparator;
        }
    }

    // The tokens point into text, which outlives the list.
    std::list<char *> components;
    for (char *pszToken = strtok(const_cast<char *>(TextOf(text)), kPathSeparators);
         pszToken != nullptr;
         pszToken = strtok(nullptr, kPathSeparators)) {
        if (pszToken[0] != kFullStop) {
            components.push_back(pszToken);
        } else if (pszToken[1] == kFullStop) {
            components.pop_back(); // Yes, a leading ".." pops an empty list, as the binary does.
        }
    }

    Clear();
    for (auto it = components.begin(); it != components.end(); ++it) {
        if (it != components.begin()) {
            *this += kPathSeparator;
        }
        *this += HxStr(*it);
    }
}

// 0x004e5168
const HxStr &FilePath::RelativeToRoot() const {
    if (mLen == 0) {
        return *this;
    }
    HxStr root(sRoot);
    HxStr path(*this);
    std::list<char *> rootComponents;
    std::list<char *> pathComponents;
    for (char *pszToken = strtok(const_cast<char *>(TextOf(root)), kPathSeparators);
         pszToken != nullptr;
         pszToken = strtok(nullptr, kPathSeparators)) {
        rootComponents.push_back(pszToken);
    }
    for (char *pszToken = strtok(const_cast<char *>(TextOf(path)), kPathSeparators);
         pszToken != nullptr;
         pszToken = strtok(nullptr, kPathSeparators)) {
        pathComponents.push_back(pszToken);
    }

    // The list sizes are counted by walking the nodes each time, as the binary does.
    while (rootComponents.size() != 0 && pathComponents.size() != 0 &&
           strcmp(rootComponents.front(), pathComponents.front()) == 0) {
        rootComponents.pop_front();
        pathComponents.pop_front();
    }

    static HxStr sRelative;
    sRelative.Clear();
    while (rootComponents.size() != 0) {
        if (sRelative.mLen != 0) {
            sRelative += HxStr(kPathSeparators);
        }
        sRelative += HxStr(kParentDirectory);
        rootComponents.pop_front();
    }
    while (pathComponents.size() != 0) {
        if (sRelative.mLen != 0) {
            sRelative += HxStr(kPathSeparators);
        }
        sRelative += HxStr(pathComponents.front());
        pathComponents.pop_front();
    }
    return sRelative;
}

// 0x004e7bc0
void FilePath::Print(FailSink &sink) const {
    sink.Format("\"%s\"", TextOf(*this));
}

// 0x004e7bf8
void FilePath::Save(Stream &stream) const {
    const HxStr &relative = RelativeToRoot();
    stream.WriteBytes(TextOf(relative), relative.mLen + 1);
}

// 0x004e7c50
void FilePath::Load(Stream &stream) {
    HxStr name;
    stream.ReadString(name);
    SetFromRoot(name);
}

// 0x004e78d8
void FilePath::SetRoot(const HxStr &root) {
    sRoot.HxStr::operator=(root);
    sRoot.Normalize();
}

} // namespace Rnd
