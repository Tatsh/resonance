#include <ctype.h>
#include <list>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <vector>

#include "os/async.h"
#include "os/failsink.h"
#include "os/genpath.h"
#include "os/hxstr.h"
#include "os/mem.h"
#include "os/zone.h"
#include "rnd/filepath.h"
#include "rnd/stream.h"
#include "rnd/tex.h"
#include "rndartt/abitmap.h"
#include "rndartt/acanvas.h"
#include "rndartt/apalette.h"

namespace Rnd {

namespace {

// The tag every release in this file bills to, and the file name its asserts record.
constexpr char kTexFileName[] = "rndtex.cpp";

// Line 610 of rndtex.cpp, which FreeLoadedBitmaps() passes to the tagged release.
constexpr int kFreeBitmapLine = 0x262;

// Line 205 of rndtex.cpp, which AllocateBitmapFromStream() passes to the tagged allocation.
constexpr int kBlankBitmapLine = 0xcd;

// The path buffers the mip loader and the read queue work in.
constexpr int kMaxPathLength = 0x100;

// The files the mip loader reads. A mip level n is the base name with "_mn" inserted before the
// extension, and every file is read as the compressed cache copy of its bitmap.
constexpr char kMipSuffixFormat[] = "_m%d";
constexpr char kCacheExtension[] = ".abm";
constexpr char kCompressedSuffix[] = ".gz";

// Texture flag bits AllocateBitmapFromStream() and the mip loader test. The first enables the
// numbered mip files, and the second makes a blank level three faces wide and two faces tall.
constexpr int kTexFlagMipChain = 0x04;
constexpr int kTexFlagCubeMap = 0x40;
constexpr int kCubeMapWidthFactor = 3;
constexpr int kCubeMapHeightFactor = 2;

// The only revision Save() writes, and the highest Load() accepts.
constexpr int kTexRevision = 4;

// The revision that stores the width and the height as 16-bit values, the last revision that
// stores a spare byte after the flags, and the first that stores mMipSelect.
constexpr int kShortSizeRevision = 1;
constexpr int kLastRevisionWithSpareByte = 2;
constexpr int kFirstRevisionWithMipSelect = 4;

constexpr char kIntFormat[] = "%d";

// A blank level of this depth or less is indexed and carries a palette.
constexpr int kMaxIndexedBitsPerPixel = 8;

// The pixels of a loaded block start on a quadword boundary.
constexpr uintptr_t kPixelAlignment = 16;

// The allocation tag every texture block is billed to.
constexpr char kTexTag[] = "Rnd::Tex";

// The lock flag GetBitmapInfo() passes when it walks every level, which requests a read-back.
constexpr int kLockMipReadBack = 1;

// The character that follows a drive letter in an absolute path.
constexpr char kDriveSeparator = ':';

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

// Texture flag bits OnMipLoaded() tests before ABitmap::SetPaletteAlphaFromLowByte(). Flag 0x10
// wins when both are set.
constexpr int kTexFlagPaletteAlpha = 0x10;
constexpr int kTexFlagPaletteAlphaWhite = 0x20;

// The flag bits DumpText() lists, in its order, with the text each prints.
struct TexFlagName {
    int nBit;
    const char *pszName;
};

constexpr TexFlagName kTexFlagNames[] = {
    {kABitmapColorKeyWhite, "TransparentWhite, "},
    {kABitmapColorKeyBlack, "TransparentWhite, "}, // Yes, the binary prints the white key's name.
    {kTexFlagMipChain, "MipMaps, "},
    {kTexFlagPaletteAlpha, "GreyscaleAlpha, "},
    {kTexFlagPaletteAlphaWhite, "GreyscaleWhite, "},
    {kTexFlagCubeMap, "CubeMap, "},
};

// A loaded mip block. The bitmap header is followed by a palette and then the pixels of an indexed
// format. A direct colour format's pixels begin where the palette would.
struct ABitmapImage : ABitmap {
    APalette mImagePalette;
    unsigned char mIndexedPixels[1];
};

// The first quadword boundary at or after pStart. Aligning an address needs the address as an
// integer, which is the one place this file converts a pointer.
inline void *AlignPixels(void *pStart) {
    const uintptr_t nAddress = reinterpret_cast<uintptr_t>(pStart);
    return reinterpret_cast<void *>((nAddress + kPixelAlignment - 1) & ~(kPixelAlignment - 1));
}

// -1 unless n is a power of two, 0 for one, and 1 for a larger power of two. OnMipLoaded() tests
// only the sign.
inline int ClassifyPowerOfTwo(int n) {
    if (n <= 0) {
        return -1;
    }
    if (n == 1) {
        return 0;
    }
    while (true) {
        if ((n & 1) != 0) {
            return -1;
        }
        n >>= 1;
        if (n == 1) {
            return 1;
        }
    }
}

} // namespace

// 0x004e3dc8
Tex::Tex(const HxStr &name)
    : Object(name), mWidth(0), mHeight(0), mBitsPerPixel(0), mFlags(0), mPendingMipMask(0),
      mMipSelect(-0x80), mBitmapPath(nullptr), mZone(-1) {
}

// 0x004e7628
Tex::~Tex() {
    FreeLoadedBitmaps();
    ReleaseAllRefs();
}

// 0x004e4738
void Tex::DumpText(FailSink &sink) {
    Object::DumpText(sink);
    if (sink.mDumpLevel <= 0) {
        return;
    }

    sink.Print("[Tex]\n");
    sink.Print("width:");
    sink.Format(kIntFormat, mWidth);
    sink.Print(" height:");
    sink.Format(kIntFormat, mHeight);
    sink.Print(" bpp:");
    sink.Format(kIntFormat, mBitsPerPixel);
    sink.Print(" mipMapK:");
    sink.Format(kIntFormat, mMipSelect);
    sink.Print(" file:");
    mBitmapPath.Print(sink);
    sink.Print(" flags:");
    if (mFlags == 0) {
        sink.Print("None");
    } else {
        for (const auto &flag : kTexFlagNames) {
            if ((mFlags & flag.nBit) != 0) {
                sink.Print(flag.pszName);
            }
        }
    }
    sink.Print("\n");
}

// 0x004e4910
void Tex::Save(Stream &stream) {
    const int nRevision = kTexRevision;
    stream.Write(&nRevision, sizeof(nRevision));
    stream.Write(&mWidth, sizeof(mWidth));
    stream.Write(&mHeight, sizeof(mHeight));
    stream.Write(&mBitsPerPixel, sizeof(mBitsPerPixel));
    mBitmapPath.Save(stream);
    stream.Write(&mFlags, sizeof(mFlags));
    stream.Write(&mMipSelect, sizeof(mMipSelect));
}

// 0x004e7610
void Tex::Replace([[maybe_unused]] Object *pFrom, [[maybe_unused]] Object *pTo) {
}

// 0x004e7618
const HxStr &Tex::ClassName() const {
    return g_texClassName;
}

// 0x004e79f0
void Tex::Copy(const Object *pSource, [[maybe_unused]] unsigned nFlags) {
    const Tex *pTex = dynamic_cast<const Tex *>(pSource);
    FreeLoadedBitmaps();
    mWidth = pTex->mWidth;
    mHeight = pTex->mHeight;
    mBitsPerPixel = pTex->mBitsPerPixel;
    mBitmapPath = pTex->mBitmapPath;
    mMipSelect = pTex->mMipSelect;
    mFlags = pTex->mFlags;
    AllocateBitmapFromStream();
}

// 0x004e4a20
void Tex::Load(Stream &stream) {
    int nRevision = 0;
    stream.Read(&nRevision, sizeof(nRevision));
    if (nRevision > kTexRevision) {
        g_failSink.Report("Can't load new Tex\n");
        return;
    }

    FreeLoadedBitmaps();
    if (nRevision == kShortSizeRevision) {
        short nShortWidth = 0;
        short nShortHeight = 0;
        stream.Read(&nShortWidth, sizeof(nShortWidth));
        stream.Read(&nShortHeight, sizeof(nShortHeight));
        mWidth = nShortWidth;
        mHeight = nShortHeight;
    } else {
        stream.Read(&mWidth, sizeof(mWidth));
        stream.Read(&mHeight, sizeof(mHeight));
    }
    stream.Read(&mBitsPerPixel, sizeof(mBitsPerPixel));
    mBitmapPath.Load(stream);
    stream.Read(&mFlags, sizeof(mFlags));
    if (nRevision >= kShortSizeRevision && nRevision <= kLastRevisionWithSpareByte) {
        char cSpare = 0;
        stream.ReadBytes(&cSpare, sizeof(cSpare)); // Read and then discarded, as in the binary.
    }
    if (nRevision >= kFirstRevisionWithMipSelect) {
        stream.Read(&mMipSelect, sizeof(mMipSelect));
    }
    AllocateBitmapFromStream();
}

// 0x004e75a0
ACanvas *Tex::LockMipBitmap([[maybe_unused]] int nMip,
                            [[maybe_unused]] int nUnknown,
                            [[maybe_unused]] int nFlags) {
    return nullptr;
}

// 0x004e75f8
void Tex::UnlockMipBitmap() {
}

// 0x004e7600
void Tex::SetPalette([[maybe_unused]] APalette *pPalette, [[maybe_unused]] int nUnknown) {
}

// 0x004e7608
void Tex::SetGsPageInUse([[maybe_unused]] bool bInUse) {
}

// 0x004e7908
void Tex::SetBitmapConfig(
    int nWidth, int nHeight, int nBitsPerPixel, const HxStr &path, int nMipSelect, int nFlags) {
    mWidth = nWidth;
    mHeight = nHeight;
    mBitsPerPixel = nBitsPerPixel;
    mMipSelect = nMipSelect;
    mFlags = nFlags;
    if (FilePath::IsAbsolute(path)) {
        mBitmapPath.Set(path);
    } else {
        mBitmapPath.SetFromRoot(path);
    }
    CancelPendingMips();
    mMipHandles.clear();
}

// 0x004e4598
void Tex::RestoreSurfaces() {
    if (!mLoadedBitmaps.empty() && mLoadedBitmaps[0] != nullptr) {
        const ABitmap *pBitmap = mLoadedBitmaps[0];
        mWidth = pBitmap->mWidth;
        mHeight = pBitmap->mHeight;
        mBitsPerPixel = g_abBitmapBitsPerPixel[pBitmap->mFormat];
    }
    mMipHandles.clear();
}

// 0x007033b0
HxStr g_texClassName("Tex");

// 0x004e77f0
Tex *NewTex(const HxStr &name) {
    return new Tex(name);
}

// 0x007033a8
Tex *(*g_pfnNewTex)(const HxStr &name) = NewTex;

// 0x004e7770
Object *CreateRegisteredTex(const HxStr &name) {
    try {
        return g_pfnNewTex(name);
    } catch (...) {
        return nullptr;
    }
}

// 0x004e7878
bool Tex::IsLoadComplete() {
    return mPendingMipMask == 0;
}

// 0x004e3e48
int Tex::GetBitmapInfo(int &nWidth, int &nHeight, int &nBitsPerPixel, int &nBytes) {
    ACanvas *pCanvas = LockMipBitmap(0, 0, 0);
    if (pCanvas == nullptr) {
        return 0;
    }
    if (pCanvas->mBitmap.mPixels == nullptr) {
        return 0; // Yes, the binary returns without unlocking the level.
    }
    nWidth = pCanvas->mBitmap.mWidth;
    nHeight = pCanvas->mBitmap.mHeight;
    nBitsPerPixel = g_abBitmapBitsPerPixel[pCanvas->mBitmap.mFormat];
    UnlockMipBitmap();

    nBytes = 0;
    for (unsigned nMip = 0; nMip < mLoadedBitmaps.size(); ++nMip) {
        ACanvas *pLevel = LockMipBitmap(nMip, 0, kLockMipReadBack);
        if (pLevel != nullptr && pLevel->mBitmap.mPixels != nullptr) {
            nBytes += pLevel->mBitmap.mByteCount;
            const APalette *pPalette = pLevel->mBitmap.mPalette;
            if (pPalette != nullptr) {
                nBytes += pPalette->mEnd * sizeof(pPalette->mEntries[0]);
            }
        }
        UnlockMipBitmap();
    }
    return 1;
}

// 0x004e3fe8
void Tex::AllocateBitmapFromStream() {
    mMipHandles.clear();
    mZone = ZoneGetCurrent();

    if (mBitmapPath.mLen != 0) {
        if (!LoadMipFiles()) {
            mBitsPerPixel = 0;
            mHeight = 0;
            mWidth = 0;
            RestoreSurfaces();
        }
        return;
    }

    int nWidth = mWidth;
    int nHeight = mHeight;
    if ((mFlags & kTexFlagCubeMap) != 0) {
        nWidth *= kCubeMapWidthFactor;
        nHeight *= kCubeMapHeightFactor;
    }
    const int nFormat = ABitmap::FormatForBitsPerPixel(mBitsPerPixel);
    const int nPixelBytes = ABitmap::ComputeByteCount(nFormat, nWidth, nHeight);
    const bool bIndexed = mBitsPerPixel <= kMaxIndexedBitsPerPixel;
    const size_t nHeaderBytes = bIndexed ? sizeof(ABitmap) + sizeof(APalette) : sizeof(ABitmap);
    const size_t nBlockBytes = nPixelBytes + nHeaderBytes + kPixelAlignment - 1;

    void *pBlock = mZone == -1 ? MemAllocTagged(nBlockBytes, kTexFileName, kBlankBitmapLine) :
                                 ZoneAlloc(nBlockBytes);
    if (pBlock == nullptr) {
        return;
    }

    auto *pImage = static_cast<ABitmapImage *>(pBlock);
    APalette *pPalette = nullptr;
    void *pPixels = nullptr;
    if (bIndexed) {
        pPalette = &pImage->mImagePalette;
        pPixels = AlignPixels(pImage->mIndexedPixels);
    } else {
        pPixels = AlignPixels(&pImage->mImagePalette);
    }
    const ABitmap bitmap(pPixels, nFormat, false, nWidth, nHeight, 0);
    static_cast<ABitmap &>(*pImage) = bitmap;
    pImage->mPalette = pPalette;

    mLoadedBitmaps.push_back(pImage);
    mPendingMipMask = 0;
    RestoreSurfaces();
}

// 0x004e4208
bool Tex::LoadMipFiles() {
    mPendingMipMask = 0;
    char szBase[kMaxPathLength];
    strcpy(szBase, TextOf(mBitmapPath));

    if ((mFlags & kTexFlagMipChain) == 0) {
        return QueueMipRead(szBase) != 0;
    }

    QueueMipRead(szBase); // Yes, the binary does not test the base level's result.
    for (int nMip = 1;; ++nMip) {
        char szSuffix[kMaxPathLength];
        char szPath[kMaxPathLength];
        sprintf(szSuffix, kMipSuffixFormat, nMip);
        strcpy(szPath, szBase);
        ReplaceFileNameExtension(szPath, szSuffix);
        if (LoadBitmapFileFromPath(szPath) == 0) {
            return true;
        }
        if (QueueMipRead(szPath) == 0) {
            return false;
        }
    }
}

// 0x004e4300
int Tex::QueueMipRead(const char *pszPath) {
    char szCache[kMaxPathLength];
    strcpy(szCache, pszPath);
    BuildBitmapCacheFileName(szCache, kCacheExtension);
    char szFile[kMaxPathLength];
    strcpy(szFile, szCache);
    strcat(szFile, kCompressedSuffix);

    mMipHandles.push_back(AsyncLoadFileByPath(szFile, nullptr, 0, nullptr));
    mLoadedBitmaps.push_back(nullptr);
    mPendingMipMask |= 1 << (mMipHandles.size() - 1);
    return 1; // Yes, the binary reports success whatever the read queue returned.
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

// 0x004e5928
void Tex::OnMipLoaded(int nMip) {
    if (mLoadedBitmaps.empty()) {
        return;
    }
    ABitmap *pBitmap = mLoadedBitmaps[nMip];
    if (pBitmap == nullptr) {
        return;
    }

    auto *pImage = static_cast<ABitmapImage *>(pBitmap);
    if (pBitmap->mFormat == kABitmapFormatLinear4 || pBitmap->mFormat == kABitmapFormatLinear8 ||
        pBitmap->mFormat == kABitmapFormatRle8) {
        pBitmap->mPalette = &pImage->mImagePalette;
        pBitmap->mPixels = pImage->mIndexedPixels;
    } else {
        pBitmap->mPalette = nullptr;
        pBitmap->mPixels = &pImage->mImagePalette;
    }

    if (g_nSkipColorSwap == 0) {
        pBitmap->SwapRedBlue();
    }
    if ((mFlags & kTexFlagPaletteAlpha) != 0) {
        pBitmap->SetPaletteAlphaFromLowByte(0);
    } else if ((mFlags & kTexFlagPaletteAlphaWhite) != 0) {
        pBitmap->SetPaletteAlphaFromLowByte(1);
    }
    pBitmap->ApplyColorKey(mFlags);

    if (ClassifyPowerOfTwo(pBitmap->mWidth) < 0 || ClassifyPowerOfTwo(pBitmap->mHeight) < 0) {
        g_failSink.Report("%s (mipmap %d) is not power of 2 in width and height (%d x %d)\n",
                          TextOf(mBitmapPath.RelativeToRoot()),
                          nMip,
                          pBitmap->mWidth,
                          pBitmap->mHeight);
    }
    if (nMip <= 0) {
        return;
    }
    const int nExpectedWidth = mWidth >> nMip;
    const int nExpectedHeight = mHeight >> nMip;
    if (pBitmap->mWidth != nExpectedWidth || pBitmap->mHeight != nExpectedHeight) {
        g_failSink.Report(
            "%s (mipmap %d) is not expected width/height (got %dx%d, expected %dx%d)\n",
            TextOf(mBitmapPath.RelativeToRoot()),
            nMip,
            pBitmap->mWidth,
            pBitmap->mHeight,
            nExpectedWidth,
            nExpectedHeight);
    }
}

// 0x004e73c8
void Tex::ReloadBitmaps() {
    FreeLoadedBitmaps();
    AllocateBitmapFromStream();
}

// 0x004e7388
void *Tex::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, kTexTag);
}

// 0x004e73a8
void Tex::operator delete(void *pBlock) {
    FreeTaggedMemory(pBlock, kTexTag);
}

// 0x004e7448
Tex *NewTexThroughHook(const HxStr &name) {
    try {
        return g_pfnNewTex(name);
    } catch (...) {
        return nullptr; // The binary's handler returns null.
    }
}

// 0x004e7568
const HxStr &Tex::GetRelativeBitmapPath() const {
    return mBitmapPath.RelativeToRoot();
}

// 0x004e7cd8
bool FilePath::IsAbsolute(const HxStr &path) {
    if (path.mLen == 0) {
        return true; // Yes, the binary counts an empty path as absolute.
    }
    return path[0] == kPathSeparator || path[0] == kBackslash || path[1] == kDriveSeparator;
}

// 0x004e4648
void Tex::CancelPendingMips() {
    if (mPendingMipMask == 0) {
        return;
    }
    for (unsigned nMip = 0; nMip < mMipHandles.size(); ++nMip) {
        if (((mPendingMipMask >> nMip) & 1) != 0) {
            AsyncCancelRequest(mMipHandles[nMip]);
            mMipHandles[nMip] = 0;
        }
    }
    mMipHandles.clear(); // Yes, the binary leaves the pending mask set.
}

// 0x004e7aa8
void Tex::FreeLoadedBitmaps() {
    CancelPendingMips();
    for (const auto pBitmap : mLoadedBitmaps) {
        // Zone memory goes with its zone. Only a bitmap from the tagged heap is released here.
        if (pBitmap != nullptr && mZone == -1) {
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
