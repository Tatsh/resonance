#include "rnd/font.h"

#include <map>

#include "math/vector2.h"
#include "os/failsink.h"
#include "os/hxstr.h"
#include "os/mem.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/object.h"
#include "rnd/stream.h"
#include "rnd/tex.h"
#include "rndartt/acanvas.h"
#include "rndartt/apalette.h"

namespace Rnd {

namespace {

constexpr int kSerialVersion = 2;
constexpr char kNoObject[] = "no object";
constexpr char kFontTag[] = "Rnd::Font";

// Bits of a canvas pixel that store its alpha.
constexpr unsigned kPixelAlphaMask = 0xff000000;

// Fraction of a cell an entirely transparent cell is credited with. A space character has no ink
// to measure, and this is what gives it a width.
constexpr int kBlankCellDivisor = 4;

// Mip level the glyph scan measures, and the two further arguments it locks that level with.
constexpr int kGlyphMipLevel = 0;
constexpr int kGlyphMipLockUnknown = 0;
constexpr int kGlyphMipLockFlags = 1;

// Stage of the material whose texture supplies the atlas.
constexpr int kAtlasStage = 0;

// Titles the weight and the family are dumped under. The binary stores each table as a global of
// pointers and passes the entry to Print() rather than to Format(), with no bound check. Both
// tables sit in the same literal pool as g_fontClassName, the weight table at 0x006fecc0 and the
// family table at 0x006fecd0.
constexpr const char *kWeightTitles[] = {"Thin", "Normal", "Bold"};
constexpr const char *kFamilyTitles[] = {"Modern", "Roman", "Swiss", "Script"};

// Character set revision 0 and revision 1 take in place of one read from the stream.
constexpr char kDefaultChars[] = " !\"#$%&'()*+,-./0123456789:;<=>?@"
                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"
                                 "abcdefghijklmnopqrstuvwxyz{|}~";

/**
 * Per-character record revision 0 of a font stored.
 *
 * The title is inferred. The record is 12 bytes, which the 0x20-byte tree node of the map it
 * arrives in and the 12-byte zero fill at `0x004cea9c` both pin, and the reader establishes the
 * three types. Every value is discarded as soon as Load() has read the map, so what the two floats
 * addressed is unrecovered.
 */
struct LegacyCharInfo {
    Mat *mMat;
    float mUnknown04;
    float mUnknown08;
};

const char *NameText(const Object *pObject) {
    return pObject->mName.mStr != nullptr ? pObject->mName.mStr : "";
}

const char *StringText(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

// 0x004d07e0. A value outside the three produces nothing at all. The sink comes back out so that
// the three printers chain, which is how DumpText() reaches them.
FailSink *PrintFontType(FailSink &sink, FontType type) {
    switch (type) {
    case kFontTypeDefault:
        return sink.Print("Default");
    case kFontTypeBuiltin:
        return sink.Print("Builtin");
    case kFontTypeMaterial:
        return sink.Print("Material");
    }
    return &sink;
}

// 0x004d0858
FailSink *PrintFontWeight(FailSink &sink, FontWeight weight) {
    return sink.Print(kWeightTitles[weight]);
}

// 0x004d0898
FailSink *PrintFontFamily(FailSink &sink, FontFamily family) {
    return sink.Print(kFamilyTitles[family]);
}

void PrintObjectRef(FailSink &sink, const Object *pObject) {
    if (pObject == nullptr) {
        sink.Print(kNoObject);
        return;
    }
    sink.Format("%s", NameText(pObject));
}

void WriteObjectRef(Stream &stream, const Object *pObject) {
    if (pObject == nullptr) {
        const char chTerminator = '\0';
        stream.WriteBytes(&chTerminator, 1);
        return;
    }
    stream.WriteBytes(NameText(pObject), pObject->mName.mLen + 1);
}

void WriteString(Stream &stream, const HxStr &text) {
    stream.WriteBytes(StringText(text), text.mLen + 1);
}

// 0x004cab18
Stream &operator>>(Stream &stream, LegacyCharInfo &info) {
    HxStr matName(nullptr);
    stream.ReadString(matName);
    info.mMat = dynamic_cast<Mat *>(g_manager.Find(matName));
    stream.Read(&info.mUnknown04, sizeof(info.mUnknown04));
    stream.Read(&info.mUnknown08, sizeof(info.mUnknown08));
    return stream;
}

// 0x004ce9c0
Stream &operator>>(Stream &stream, std::map<char, LegacyCharInfo> &charMap) {
    int nCount = 0;
    stream.Read(&nCount, sizeof(nCount));
    while (nCount > 0) {
        char chKey = '\0';
        stream.ReadBytes(&chKey, sizeof(chKey));
        stream >> charMap[chKey];
        --nCount;
    }
    return stream;
}

// Walk x from nFrom toward nTo and report the first column with any pixel whose alpha byte is set,
// yielding nTo when there is none. The compiler emitted this twice inside ComputeCharUV(), once
// forwards across the cell and once backwards across it, which is why the direction is derived
// from the two bounds rather than passed.
inline int ScanForInkedColumn(ACanvas &canvas, int nFrom, int nTo, int nTop, int nBottom) {
    const bool bForward = nFrom < nTo;
    int x = nFrom;
    while (x != nTo) {
        for (int y = nTop; y < nBottom; ++y) {
            if ((canvas.GetPixel(x, y) & kPixelAlphaMask) != 0) {
                return x;
            }
        }
        x += bForward ? 1 : -1;
    }
    return x;
}

} // namespace

// 0x004cb1e8
Font::Font(const HxStr &name)
    : Object(name), mType(kFontTypeDefault), mHeight(12), mWeight(kFontWeightNormal), mItalic(0),
      mFamily(kFontFamilyRoman), mMat(nullptr), mRows(1.0f), mCols(1.0f), mSize(0.0f),
      mSpace(0.0f) {
}

// 0x004d0738
void Font::RemoveMatRef() {
    if (mMat != nullptr) {
        mMat->RemoveRef(this);
    }
}

// 0x004cee70
Font::~Font() {
    RemoveMatRef();
    ReleaseAllRefs();
}

// 0x004cefd0
const HxStr &Font::ClassName() const {
    return g_fontClassName;
}

// 0x004cedd0
void Font::SetBuiltin(
    int nHeight, FontWeight weight, int nItalic, FontFamily family, const HxStr &name) {
    mHeight = nHeight;
    mWeight = weight;
    mItalic = nItalic;
    mFamily = family;
    mName = name;
}

// 0x004cef88
void Font::GetBuiltin(int *nHeightOut,
                      FontWeight *weightOut,
                      int *nItalicOut,
                      FontFamily *familyOut,
                      HxStr &nameOut) {
    *nHeightOut = mHeight;
    *weightOut = mWeight;
    *nItalicOut = mItalic;
    *familyOut = mFamily;
    nameOut = mName;
}

// 0x004ca470
void Font::DumpText(FailSink &sink) {
    Object::DumpText(sink);
    if (sink.mDumpLevel <= 0) {
        return;
    }

    sink.Print("[Font]\n");
    sink.Print("type:");
    PrintFontType(sink, mType);

    if (mType == kFontTypeBuiltin) {
        sink.Print(" height:");
        sink.Format("%d", mHeight);
        sink.Print(" weight:");
        PrintFontWeight(sink, mWeight);
        sink.Print("italic:");
        sink.Print(mItalic != 0 ? "true" : "false");
        sink.Print(" family:");
        PrintFontFamily(sink, mFamily);
        sink.Print(" name:");
        sink.Format("%s", StringText(mName));
        sink.Print("\n");
        return;
    }

    if (mType != kFontTypeMaterial) {
        return;
    }

    sink.Print(" mat:");
    PrintObjectRef(sink, mMat);
    sink.Print(" rows:");
    sink.Format("%.2f", mRows);
    sink.Print(" cols:");
    sink.Format("%.2f", mCols);
    sink.Print("\n");
    sink.Print("size:");
    sink.Format("%.2f", mSize);
    sink.Print(" space:");
    sink.Format("%.2f", mSpace);
    sink.Print("\n");
}

// 0x004ca720
void Font::Save(Stream &stream) {
    const int nVersion = kSerialVersion;
    stream.Write(&nVersion, sizeof(nVersion));
    stream.Write(&mType, sizeof(mType));
    stream.Write(&mHeight, sizeof(mHeight));
    stream.Write(&mWeight, sizeof(mWeight));

    const char chItalic = static_cast<char>(mItalic);
    stream.WriteBytes(&chItalic, sizeof(chItalic));

    stream.Write(&mFamily, sizeof(mFamily));
    WriteString(stream, mName);
    WriteObjectRef(stream, mMat);
    stream.Write(&mRows, sizeof(mRows));
    stream.Write(&mCols, sizeof(mCols));
    stream.Write(&mSize, sizeof(mSize));
    stream.Write(&mSpace, sizeof(mSpace));
    WriteString(stream, mChars);
}

// 0x004d0690
void Font::Replace(Object *pFrom, Object *pTo) {
    if (mMat != pFrom) {
        return;
    }
    RemoveMatRef();
    if (mMat != nullptr) {
        mMat = pTo != nullptr ? dynamic_cast<Mat *>(pTo) : nullptr;
    }
    if (mMat != nullptr) {
        mMat->AddRef(this);
    }
}

// 0x004cb0b8
void Font::Copy(const Object *pSource, [[maybe_unused]] unsigned nFlags) {
    // The cast result is dereferenced with no null check, so a pSource of another class faults here
    // rather than being rejected. No base implementation is invoked and nFlags is never read.
    const Font *pFont = pSource != nullptr ? dynamic_cast<const Font *>(pSource) : nullptr;
    RemoveMatRef();

    mType = pFont->mType;
    mHeight = pFont->mHeight;
    mWeight = pFont->mWeight;
    mItalic = pFont->mItalic;
    mFamily = pFont->mFamily;
    mName = pFont->mName;
    mMat = pFont->mMat;
    mRows = pFont->mRows;
    mCols = pFont->mCols;
    mSize = pFont->mSize;
    mSpace = pFont->mSpace;
    mChars = pFont->mChars;

    if (mMat != nullptr) {
        mMat->AddRef(this);
    }
    mCharMap.clear();
}

// 0x004cac20
void Font::Load(Stream &stream) {
    int nVersion = 0;
    stream.Read(&nVersion, sizeof(nVersion));
    if (nVersion > kSerialVersion) {
        g_failSink.Report("Can't load new Font\n");
        g_failSink.mAbortProc();
    }

    RemoveMatRef();

    stream.Read(&mType, sizeof(mType));
    stream.Read(&mHeight, sizeof(mHeight));
    stream.Read(&mWeight, sizeof(mWeight));

    char chItalic = '\0';
    stream.ReadBytes(&chItalic, sizeof(chItalic));
    mItalic = chItalic != '\0';

    stream.Read(&mFamily, sizeof(mFamily));
    stream.ReadString(mName);

    if (nVersion > 0) {
        HxStr matName(nullptr);
        stream.ReadString(matName);
        mMat = dynamic_cast<Mat *>(g_manager.Find(matName));

        if (nVersion < 2) {
            // The two grid dimensions arrived as integers and are widened here.
            int nRows = 0;
            int nCols = 0;
            stream.Read(&nRows, sizeof(nRows));
            stream.Read(&nCols, sizeof(nCols));
            mRows = static_cast<float>(nRows);
            mCols = static_cast<float>(nCols);
        } else {
            stream.Read(&mRows, sizeof(mRows));
            stream.Read(&mCols, sizeof(mCols));
        }

        stream.Read(&mSize, sizeof(mSize));
        stream.Read(&mSpace, sizeof(mSpace));
    } else {
        // Revision 0 stored a character map of its own, one material and two floats per character.
        // Every entry is read and then discarded, and neither the material nor the atlas of this
        // build is in such a file at all, so all five of those fields survive the load unchanged.
        std::map<char, LegacyCharInfo> discarded;
        stream >> discarded;
    }

    if (nVersion >= 2) {
        stream.ReadString(mChars);
    } else {
        mChars = kDefaultChars;
    }

    if (mMat != nullptr) {
        mMat->AddRef(this);
    }
    mCharMap.clear();
}

// 0x004ca050
void Font::ComputeCharUV(int nRow, int nCol, CharInfo &infoOut) {
    ACanvas *pCanvas = nullptr;
    Tex *pTex = nullptr;
    if (mMat != nullptr && !mMat->mStages.empty()) {
        pTex = mMat->mStages[kAtlasStage].mTex;
        if (pTex != nullptr) {
            pCanvas = pTex->LockMipBitmap(kGlyphMipLevel, kGlyphMipLockUnknown, kGlyphMipLockFlags);
        }
    }

    if (pTex == nullptr) {
        infoOut.mAdvance = 0.0f;
        infoOut.mU0 = 0.0f;
        infoOut.mV0 = 0.0f;
        infoOut.mU1 = 0.0f;
        infoOut.mV1 = 0.0f;
        return;
    }

    // With no surface to scan the cell degenerates to a single column that is entirely ink, which
    // gives every character the full cell advance.
    int nCellLeft = 0;
    int nCellRight = 1;
    int nInkFirst = 0;
    int nInkLast = 1;

    if (pCanvas != nullptr) {
        // The cell bounds are derived in the surface's own pixels and truncated to integers, so a
        // grid that does not divide the surface evenly loses the remainder.
        nCellLeft = static_cast<int>(static_cast<float>(nCol * pCanvas->mBitmap.mWidth) / mCols);
        nCellRight =
            static_cast<int>(static_cast<float>((nCol + 1) * pCanvas->mBitmap.mWidth) / mCols);
        const int nCellTop =
            static_cast<int>(static_cast<float>(nRow * pCanvas->mBitmap.mHeight) / mRows);
        const int nCellBottom =
            static_cast<int>(static_cast<float>((nRow + 1) * pCanvas->mBitmap.mHeight) / mRows);

        nInkFirst = ScanForInkedColumn(*pCanvas, nCellLeft, nCellRight, nCellTop, nCellBottom);
        nInkLast =
            ScanForInkedColumn(*pCanvas, nCellRight - 1, nCellLeft - 1, nCellTop, nCellBottom) + 1;
        pTex->UnlockMipBitmap();
    }

    const int nCellSpan = nCellRight - nCellLeft;
    if (nInkFirst >= nInkLast) {
        // Neither scan found an opaque pixel, which is the case a space character produces.
        nInkFirst = nCellLeft;
        nInkLast = nCellLeft + nCellSpan / kBlankCellDivisor;
    }

    const float flCellSpan = static_cast<float>(nCellSpan);
    const float flInkSpan = static_cast<float>(nInkLast - nInkFirst);
    const float flBearing = static_cast<float>(nInkFirst - nCellLeft);
    const float flInkFraction = flInkSpan / flCellSpan;

    infoOut.mAdvance = (mSize * flInkSpan) / flCellSpan;
    infoOut.mU0 = (static_cast<float>(nCol) + flBearing / flCellSpan) / mCols;
    infoOut.mV0 = static_cast<float>(nRow) / mRows;
    infoOut.mU1 = infoOut.mU0 + flInkFraction / mCols;
    infoOut.mV1 = infoOut.mV0 + 1.0f / mRows;
}

// 0x004ca978
void Font::BuildCharMap() {
    int nRow = 0;
    int nCol = 0;
    for (unsigned i = 0; i < mChars.mLen; ++i) {
        ComputeCharUV(nRow, nCol, mCharMap[mChars.mStr[i]]);
        ++nCol;
        if (nCol >= static_cast<int>(mCols)) {
            nCol = 0;
            ++nRow;
        }
    }
}

// 0x004d0600
void Font::SetMat(Mat *pMat) {
    RemoveMatRef();
    mMat = pMat;
    OnChanged();
}

// 0x004d0648
void Font::SetSize(float flSize) {
    RemoveMatRef();
    mSize = flSize;
    OnChanged();
}

// 0x004d0768
void Font::OnChanged() {
    if (mMat != nullptr) {
        mMat->AddRef(this);
    }
    mCharMap.clear();
}

// 0x004d0988
float Font::GetCharAdvance(char ch) {
    if (mCharMap.empty()) {
        BuildCharMap();
    }
    const std::map<char, CharInfo>::iterator it = mCharMap.find(ch);
    if (it == mCharMap.end()) {
        return 0.0f;
    }
    return it->second.mAdvance;
}

// 0x004d08d8
void Font::GetCharUV(char ch, Vector2 &uv0, Vector2 &uv1) {
    if (mCharMap.empty()) {
        BuildCharMap();
    }
    const std::map<char, CharInfo>::iterator it = mCharMap.find(ch);
    if (it == mCharMap.end()) {
        uv0.x = 0.0f;
        uv0.y = 0.0f;
        uv1.x = 0.0f;
        uv1.y = 0.0f;
        return;
    }
    uv0.x = it->second.mU0;
    uv0.y = it->second.mV0;
    uv1.x = it->second.mU1;
    uv1.y = it->second.mV1;
}

// 0x004cecc0
void *Font::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, kFontTag);
}

// 0x004cece0
void Font::operator delete(void *pBlock) {
    FreeTaggedMemory(pBlock, kFontTag);
}

// 0x004cf060
Font *NewFont(const HxStr &name) {
    // The binary bills the allocation to the tag "Rnd::Font" and requests exactly 0x60 bytes.
    return new Font(name);
}

// 0x006fecb8. Null in the image until RegisterFontClass() or Rnd::Manager::Init() fills it,
// unlike g_pfnNewText.
Font *(*g_pfnNewFont)(const HxStr &name);

// 0x004ced40
Font *NewFontThroughHook(const HxStr &name) {
    return g_pfnNewFont(name);
}

// 0x004cefe0
Object *CreateRegisteredFont(const HxStr &name) {
    return g_pfnNewFont(name);
}

// 0x004ced00
void RegisterFontClass() {
    g_pfnNewFont = NewFont;
    g_manager.RegisterClass(g_fontClassName, CreateRegisteredFont);
}

// 0x006fecb0
HxStr g_fontClassName("Font");

} // namespace Rnd
