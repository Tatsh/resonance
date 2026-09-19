#include "rnd/text.h"

#include <list>
#include <string.h>
#include <vector>

#include "math/color.h"
#include "os/failsink.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/collideable.h"
#include "rnd/drawable.h"
#include "rnd/font.h"
#include "rnd/manager.h"
#include "rnd/mesh.h"
#include "rnd/meshface.h"
#include "rnd/meshvert.h"
#include "rnd/object.h"
#include "rnd/stream.h"
#include "rnd/transformable.h"

namespace Rnd {

namespace {

constexpr int kSerialVersion = 6;
constexpr char kNoObject[] = "no object";

/** Alignment word the constructor starts with. */
constexpr int kDefaultAlign = kTextAlignTop | kTextAlignLeft;

/** Wrap width the constructor starts with, and the one a file below revision 4 restores. */
constexpr float kDefaultWrapWidth = 100.0f;

/** Widest wrap width a file below revision 5 may request. */
constexpr float kMaxLoadedWrapWidth = 1000.0f;

/** Vertices and triangles one glyph occupies. */
constexpr int kVertsPerGlyph = 4;
constexpr int kFacesPerGlyph = 2;

/**
 * Alignment words the six values of the revision 2 alignment enumeration map to.
 *
 * The table is the six words at `0x008222f8`, which Load() copies onto its own frame before
 * indexing. An index outside the six reads past the copy.
 */
constexpr int kLegacyAlignMap[] = {kTextAlignTop | kTextAlignLeft,
                                   kTextAlignTop | kTextAlignCenter,
                                   kTextAlignTop | kTextAlignRight,
                                   kTextAlignBottom | kTextAlignLeft,
                                   kTextAlignBottom | kTextAlignCenter,
                                   kTextAlignBottom | kTextAlignRight};

/** Scale a file below revision 2 applies to the y component of its position pair. */
constexpr float kLegacyPositionYScale = 0.75f;

/**
 * Buffers ApplyWordWrap() wraps inside.
 *
 * The recovered extent of each is 0x3f0 bytes. The second buffer starts on a 16-byte boundary,
 * which is where the source constant loses three bits, so anything from 1001 through 1008 produces
 * the same frame and the extent is an upper bound rather than the constant itself.
 */
constexpr int kWrapBufferSize = 1008;

const char *NameText(const Object *pObject) {
    return pObject->mName.mStr != nullptr ? pObject->mName.mStr : "";
}

const char *StringText(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
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

// 0x004d0450
FailSink &PrintAlign(FailSink &sink, int nAlign) {
    if ((nAlign & kTextAlignTop) != 0) {
        sink.Print("Top");
    } else if ((nAlign & kTextAlignMiddle) != 0) {
        sink.Print("Middle");
    } else if ((nAlign & kTextAlignBottom) != 0) {
        sink.Print("Bottom");
    }

    if ((nAlign & kTextAlignLeft) != 0) {
        sink.Print("Left");
    } else if ((nAlign & kTextAlignCenter) != 0) {
        sink.Print("Center");
    } else if ((nAlign & kTextAlignRight) != 0) {
        sink.Print("Right");
    }
    return sink;
}

// The vertex the glyph mesh grows by. Both padded vectors take 1.0 in their fourth word and the
// colour is opaque white, which is the default-constructed vertex rather than a zeroed one.
MeshVert BlankVert() {
    MeshVert vert;
    vert.mPoint.x = 0.0f;
    vert.mPoint.y = 0.0f;
    vert.mPoint.z = 0.0f;
    vert.mPoint.w = 1.0f;
    vert.mNorm.x = 0.0f;
    vert.mNorm.y = 0.0f;
    vert.mNorm.z = 0.0f;
    vert.mNorm.w = 1.0f;
    vert.mColor.r = 1.0f;
    vert.mColor.g = 1.0f;
    vert.mColor.b = 1.0f;
    vert.mColor.a = 1.0f;
    vert.mTex1.x = 0.0f;
    vert.mTex1.y = 0.0f;
    vert.mTex2.x = 0.0f;
    vert.mTex2.y = 0.0f;
    return vert;
}

MeshFace BlankFace() {
    MeshFace face;
    face.mV1 = 0;
    face.mV2 = 0;
    face.mV3 = 0;
    return face;
}

} // namespace

// 0x004c89c0
Text::Text(const HxStr &name)
    : Object(name), mAlign(kDefaultAlign), mFont(nullptr), mWordWrap(0),
      mWrapWidth(kDefaultWrapWidth), mMesh(nullptr), mZTest(0) {
    mColor.r = 1.0f;
    mColor.g = 1.0f;
    mColor.b = 1.0f;
    mColor.a = 1.0f;
}

// 0x004cf958
void Text::RemoveObjectRefs() {
    if (mFont != nullptr) {
        mFont->RemoveRef(this);
    }
    delete mMesh;
    mMesh = nullptr;
}

// 0x004cf9b8
void Text::AddObjectRefs() {
    if (mFont != nullptr) {
        mFont->AddRef(this);
    }
    RebuildText();
}

// 0x004cf2b8
Text::~Text() {
    RemoveObjectRefs();
    ReleaseAllRefs();
}

// 0x004cf760
const HxStr &Text::ClassName() const {
    return g_textClassName;
}

// 0x004cf9f8
void Text::RebuildText() {
    if (mWordWrap != 0 && mFont != nullptr) {
        mText = ApplyWordWrap(mPreWrapText);
    } else {
        mText = mPreWrapText;
    }
    BuildGlyphMesh();
}

// 0x004cff48
void Text::SetAlign(int nAlign) {
    mAlign = nAlign;
    RebuildText();
}

// 0x004d0168
void Text::SetText(const HxStr &text) {
    mPreWrapText = text;
    RebuildText();
}

// 0x004cfab8
void Text::SetWordWrap(int nWordWrap) {
    mWordWrap = nWordWrap;
    RebuildText();
}

// 0x004cfb78
void Text::SetWrapWidth(float flWrapWidth) {
    mWrapWidth = flWrapWidth;
    RebuildText();
}

// 0x004cfde0
void Text::SetFont(Font *pFont) {
    if (mFont != nullptr) {
        mFont->RemoveRef(this);
    }
    mFont = pFont;
    if (mFont != nullptr) {
        mFont->AddRef(this);
    }
    RebuildText();
}

// 0x004cfec8
void Text::SetColor(const Color &color) {
    mColor = color;
    if (mMesh == nullptr) {
        return;
    }
    std::vector<MeshVert> &verts = mMesh->mVertsOwner->mVerts;
    for (std::vector<MeshVert>::iterator it = verts.begin(); it != verts.end(); ++it) {
        it->mColor = color;
    }
    mMesh->SyncChanged(Mesh::kSyncColors);
}

// 0x004d0378
void Text::SetShowing(int nShowing) {
    if (nShowing == mShowing) {
        return;
    }
    mShowing = nShowing;
    if (nShowing == 0) {
        delete mMesh;
        mMesh = nullptr;
        return;
    }
    BuildGlyphMesh();
}

// 0x004d0328
void Text::SetHighlight(int nHighlight) {
    Drawable::SetHighlight(nHighlight);
    if (mMesh != nullptr) {
        mMesh->SetHighlight(nHighlight);
    }
}

// 0x004d02f8
int Text::DrawSelf() {
    if (mMesh != nullptr) {
        mMesh->Draw();
    }
    return 1;
}

// 0x004d02a0
void Text::SetBillboard(int nBillboard) {
    Transformable::SetBillboard(nBillboard);
    if (mMesh != nullptr) {
        mMesh->SetBillboard(nBillboard);
    }
}

// 0x004d03e0
int Text::UpdateWorldXfm(Transformable *pParent, int nForce) {
    const int nMoved = Transformable::UpdateWorldXfm(pParent, nForce);
    if (mMesh != nullptr) {
        mMesh->UpdateWorldXfm(this, nMoved);
    }
    return nMoved;
}

// 0x004c7ef8
void Text::Collide(const Ray &ray, HitSink &sink) {
    if (mShowing == 0) {
        return;
    }

    if (mMesh != nullptr) {
        // The hits the mesh is about to append start after whatever the collector already stored.
        std::list<Hit>::iterator itLast = sink.mHits.end();
        --itLast;
        mMesh->Collide(ray, sink);
        for (std::list<Hit>::iterator it = ++itLast; it != sink.mHits.end(); ++it) {
            it->mObject = this;
        }
    }

    Collideable::Collide(ray, sink);
}

// 0x004c7fc0
void Text::DumpText(FailSink &sink) {
    Object::DumpText(sink);
    Drawable::DumpText(sink);
    Collideable::DumpText(sink);
    Transformable::DumpText(sink);
    if (sink.mDumpLevel <= 0) {
        return;
    }

    sink.Print("[Text]\n");
    sink.Print("font:");
    PrintObjectRef(sink, mFont);
    sink.Print(" align:");
    PrintAlign(sink, mAlign);
    sink.Print("\n");

    sink.Print("preWrapText:");
    sink.Format("%s", StringText(mPreWrapText));
    sink.Print("\n");

    sink.Print("color:");
    sink.Print("(r:");
    sink.Format("%.2f", mColor.r);
    sink.Print(" g:");
    sink.Format("%.2f", mColor.g);
    sink.Print(" b:");
    sink.Format("%.2f", mColor.b);
    sink.Print(" a:");
    sink.Format("%.2f", mColor.a);
    sink.Print(")");
    sink.Print(" word wrap:");
    sink.Print(mWordWrap != 0 ? "true" : "false");
    sink.Print(" wrap width:");
    sink.Format("%.2f", mWrapWidth);
    sink.Print("\n");

    if (sink.mDumpLevel < 2) {
        return;
    }

    sink.Print("text:");
    sink.Format("%s", StringText(mText));
    sink.Print("mesh:");
    PrintObjectRef(sink, mMesh);
    sink.Print("\n");
}

// 0x004c8330
void Text::Save(Stream &stream) {
    const int nVersion = kSerialVersion;
    stream.Write(&nVersion, sizeof(nVersion));

    Drawable::Save(stream);
    Collideable::Save(stream);
    Transformable::Save(stream);

    WriteObjectRef(stream, mFont);
    stream.Write(&mAlign, sizeof(mAlign));
    stream.WriteBytes(StringText(mPreWrapText), mPreWrapText.mLen + 1);
    stream.Write(&mColor.r, sizeof(mColor.r));
    stream.Write(&mColor.g, sizeof(mColor.g));
    stream.Write(&mColor.b, sizeof(mColor.b));
    stream.Write(&mColor.a, sizeof(mColor.a));

    const char chWordWrap = static_cast<char>(mWordWrap);
    stream.WriteBytes(&chWordWrap, sizeof(chWordWrap));
    stream.Write(&mWrapWidth, sizeof(mWrapWidth));

    const char chZTest = static_cast<char>(mZTest);
    stream.WriteBytes(&chZTest, sizeof(chZTest));
}

// 0x004cf888
void Text::Replace(Object *pFrom, Object *pTo) {
    Drawable::Replace(pFrom, pTo);
    Collideable::Replace(pFrom, pTo);
    Transformable::Replace(pFrom, pTo);

    if (mFont != pFrom) {
        return;
    }
    if (mFont != nullptr) {
        mFont->RemoveRef(this);
    }
    if (mFont != nullptr) {
        mFont = pTo != nullptr ? dynamic_cast<Font *>(pTo) : nullptr;
    }
    if (mFont != nullptr) {
        mFont->AddRef(this);
    }
}

// 0x004cfca8
void Text::Copy(const Object *pSource, unsigned nFlags) {
    // The cast result is dereferenced with no null check, so a pSource of another class faults here
    // rather than being rejected.
    const Text *pText = pSource != nullptr ? dynamic_cast<const Text *>(pSource) : nullptr;

    Drawable::Copy(pSource, nFlags);
    Collideable::Copy(pSource, nFlags);
    Transformable::Copy(pSource, nFlags);

    RemoveObjectRefs();

    // mColor is not among the fields copied, so a copy retains whatever colour it already had.
    mFont = pText->mFont;
    mAlign = pText->mAlign;
    mPreWrapText = pText->mPreWrapText;
    mWordWrap = pText->mWordWrap;
    mWrapWidth = pText->mWrapWidth;
    mZTest = pText->mZTest;

    AddObjectRefs();
}

// 0x004c8560
void Text::Load(Stream &stream) {
    int nVersion = 0;
    stream.Read(&nVersion, sizeof(nVersion));
    if (nVersion > kSerialVersion) {
        g_failSink.Report("Can't load new Text\n");
        g_failSink.mAbortProc();
    }

    Drawable::Load(stream);
    Collideable::Load(stream);
    if (nVersion >= 2) {
        Transformable::Load(stream);
    }

    RemoveObjectRefs();

    HxStr fontName(nullptr);
    stream.ReadString(fontName);
    mFont = dynamic_cast<Font *>(g_manager.Find(fontName));

    if (nVersion < 3) {
        int nLegacyAlign = 0;
        stream.Read(&nLegacyAlign, sizeof(nLegacyAlign));
        mAlign = kLegacyAlignMap[nLegacyAlign];
    } else {
        int nAlign = 0;
        stream.Read(&nAlign, sizeof(nAlign));
        mAlign = nAlign;
    }

    if (nVersion < 2) {
        // A plain screen position stands in for the transform this revision does not store. The y
        // component becomes the z row of the local transform, negated and scaled.
        float flX = 0.0f;
        float flY = 0.0f;
        stream.Read(&flX, sizeof(flX));
        stream.Read(&flY, sizeof(flY));
        mLocalXfm[3][0] = flX;
        mLocalXfm[3][1] = 0.0f;
        mLocalXfm[3][2] = -flY * kLegacyPositionYScale;
        mLocalXfm[3][3] = 1.0f;
        mDirty = 1;
    }

    stream.ReadString(mPreWrapText);

    if (nVersion > 0) {
        stream.Read(&mColor.r, sizeof(mColor.r));
        stream.Read(&mColor.g, sizeof(mColor.g));
        stream.Read(&mColor.b, sizeof(mColor.b));
        stream.Read(&mColor.a, sizeof(mColor.a));
    }

    if (nVersion >= 4) {
        char chWordWrap = '\0';
        stream.ReadBytes(&chWordWrap, sizeof(chWordWrap));
        mWordWrap = chWordWrap != '\0';
        stream.Read(&mWrapWidth, sizeof(mWrapWidth));
        if (nVersion < 5) {
            if (mWrapWidth < 0.0f) {
                mWrapWidth = kDefaultWrapWidth;
            } else if (kMaxLoadedWrapWidth < mWrapWidth) {
                mWrapWidth = kMaxLoadedWrapWidth;
            }
        }
    } else {
        mWordWrap = 0;
        mWrapWidth = kDefaultWrapWidth;
    }

    if (nVersion == 5) {
        // Revision 5 alone stored the wrapped text. AddObjectRefs() derives it again immediately,
        // so what the file supplies is overwritten unread.
        stream.ReadString(mText);
    }

    if (nVersion >= 5) {
        char chZTest = '\0';
        stream.ReadBytes(&chZTest, sizeof(chZTest));
        mZTest = chZTest != '\0';
    }

    AddObjectRefs();
}

// 0x004c9278
float Text::MeasureRun(const char *pText, int nCount) {
    float flWidth = 0.0f;
    if (mFont != nullptr) {
        for (int i = 0; i < nCount; ++i) {
            flWidth += mFont->GetCharAdvance(pText[i]);
        }
    }
    // The running total is truncated to a whole number before every comparison, which drops the
    // fractional part of an accumulated advance rather than rounding it.
    return static_cast<float>(static_cast<int>(flWidth));
}

// 0x004c9278
int Text::FindLineBreak(const char *pText) {
    if (*pText == '\n') {
        return 0;
    }

    const char *pNewline = strchr(pText, '\n');
    if (pNewline == nullptr) {
        pNewline = pText + strlen(pText);
    }
    const char *pSpace = strchr(pText, ' ');
    if (pSpace == nullptr) {
        pSpace = pText + strlen(pText);
    }

    int nCount = static_cast<int>(pNewline - pText) + 1;
    if (MeasureRun(pText, nCount - 1) <= mWrapWidth) {
        return nCount - 1;
    }

    nCount = static_cast<int>(pSpace - pText) + 1;
    if (mWrapWidth < MeasureRun(pText, nCount - 1)) {
        // Even the first word overflows, so the break falls inside it.
        while (pText != nullptr && *pText != '\0') {
            --nCount;
            if (MeasureRun(pText, nCount - 1) <= mWrapWidth) {
                return nCount;
            }
        }
        return nCount;
    }

    int nFit = nCount;
    const char *pWord = pSpace;
    for (;;) {
        if (pWord[1] == '\0' || pWord[1] == '\n') {
            return nFit;
        }

        const char *pNext = pWord + 1;
        pSpace = strchr(pNext, ' ');
        if (pSpace == nullptr) {
            const int nRest = static_cast<int>(strlen(pNext)) - 1;
            if (mWrapWidth < MeasureRun(pText, nFit + nRest)) {
                return nFit;
            }
            return nFit + nRest + 1;
        }

        const int nWord = static_cast<int>(pSpace - pNext) + 1;
        if (mWrapWidth < MeasureRun(pText, nFit + nWord - 1)) {
            return nFit;
        }
        nFit += nWord;
        pWord = pSpace;
    }
}

// 0x004c95d0
HxStr Text::ApplyWordWrap(const HxStr &text) {
    char szLine[kWrapBufferSize];
    strcpy(szLine, StringText(text));

    int nFit = FindLineBreak(szLine);
    if (static_cast<unsigned>(nFit) == text.mLen) {
        return text;
    }

    // A line whose first character is a newline reports nothing, and the routine then wraps no part
    // of the text at all rather than stepping over that newline and continuing.
    if (nFit != 0) {
        char szTail[kWrapBufferSize];
        char *pPos = szLine;
        do {
            nFit = FindLineBreak(pPos);
            if (nFit != 0) {
                char *pBreak = pPos + nFit;
                strcpy(szTail, pBreak);
                if (*pBreak != '\0' && *pBreak != '\n') {
                    pBreak[1] = '\0';
                    pBreak[0] = '\n';
                    strcat(pPos, szTail);
                } else {
                    ++nFit;
                }
            } else if (*pPos == '\n') {
                ++pPos;
            }
            pPos += nFit;
        } while (*pPos != '\0');
    }

    return HxStr(szLine);
}

// 0x004c9ca0
void Text::EmitLineGlyphs(
    float flLineY, float flLineWidth, int nCharBase, const char *pBegin, const char *pEnd) {
    float flX = 0.0f;
    if ((mAlign & kTextAlignCenter) != 0) {
        flX = -flLineWidth * 0.5f;
    } else if ((mAlign & kTextAlignRight) != 0) {
        flX = -flLineWidth;
    }

    const float flZ = -flLineY * mFont->mSize;
    std::vector<MeshVert> &verts = mMesh->mVertsOwner->mVerts;
    std::vector<MeshFace> &faces = mMesh->mFacesOwner->mFaces;
    std::vector<MeshVert>::iterator pVert = verts.begin() + nCharBase * kVertsPerGlyph;
    std::vector<MeshFace>::iterator pFace = faces.begin() + nCharBase * kFacesPerGlyph;

    for (const char *p = pBegin; p != pEnd; ++p) {
        Vector2 uv0;
        Vector2 uv1;
        mFont->GetCharUV(*p, uv0, uv1);

        const float flAdvance = mFont->GetCharAdvance(*p);
        const float flRight = flX + flAdvance;
        const float flBottom = flZ - mFont->mSize;

        pVert[0].mPoint.x = flX;
        pVert[0].mPoint.y = 0.0f;
        pVert[0].mPoint.z = flZ;
        pVert[0].mTex1 = uv0;

        pVert[1].mPoint.x = flX;
        pVert[1].mPoint.y = 0.0f;
        pVert[1].mPoint.z = flBottom;
        pVert[1].mTex1.x = uv0.x;
        pVert[1].mTex1.y = uv1.y;

        pVert[2].mPoint.x = flRight;
        pVert[2].mPoint.y = 0.0f;
        pVert[2].mPoint.z = flBottom;
        pVert[2].mTex1 = uv1;

        pVert[3].mPoint.x = flRight;
        pVert[3].mPoint.y = 0.0f;
        pVert[3].mPoint.z = flZ;
        pVert[3].mTex1.x = uv1.x;
        pVert[3].mTex1.y = uv0.y;

        pVert[0].mColor = mColor;
        pVert[1].mColor = mColor;
        pVert[2].mColor = mColor;
        pVert[3].mColor = mColor;

        const unsigned short nFirst = static_cast<unsigned short>(pVert - verts.begin());
        pFace[0].mV1 = nFirst;
        pFace[0].mV2 = nFirst + 1;
        pFace[0].mV3 = nFirst + 2;
        pFace[1].mV1 = nFirst;
        pFace[1].mV2 = nFirst + 2;
        pFace[1].mV3 = nFirst + 3;

        pVert += kVertsPerGlyph;
        pFace += kFacesPerGlyph;
        flX += flAdvance + mFont->mSpace;
    }
}

// 0x004c9780
void Text::BuildGlyphMesh() {
    delete mMesh;
    mMesh = nullptr;

    if (mShowing == 0 || mFont == nullptr || mFont->mType != kFontTypeMaterial) {
        return;
    }

    mMesh = g_pfnNewMesh(HxStr(FormatString("[%s_mesh]", NameText(this))));
    mMesh->SetBillboard(mBillboard);
    mMesh->mInternal = 1;
    mMesh->SetMaterial(mFont->mMat);

    int nLines = 0;
    int nPos = 0;
    for (;;) {
        ++nLines;
        nPos = mText.Find('\n', nPos);
        if (nPos == static_cast<int>(g_nHxStrNoPosition)) {
            break;
        }
        ++nPos;
    }

    const int nGlyphs = static_cast<int>(mText.mLen) + 1 - nLines;
    mMesh->mFacesOwner->mFaces.resize(nGlyphs * kFacesPerGlyph, BlankFace());
    mMesh->mVertsOwner->mVerts.resize(mMesh->mFacesOwner->mFaces.size() * 2, BlankVert());

    if (mZTest != 0) {
        mMesh->mZMode = Mesh::kZModeZReadOnly;
        mMesh->mZFunc = Mesh::kZFuncLess;
    } else {
        mMesh->mZMode = Mesh::kZModeDisable;
        mMesh->mZFunc = Mesh::kZFuncNever;
    }

    float flLineY = 0.0f;
    if ((mAlign & kTextAlignMiddle) != 0) {
        flLineY = static_cast<float>(-nLines) * 0.5f;
    } else if ((mAlign & kTextAlignBottom) != 0) {
        flLineY = static_cast<float>(-nLines);
    }

    int nCharBase = 0;
    float flLineWidth = 0.0f;
    const char *pLine = mText.mStr;
    const char *p = pLine;
    while (p != mText.mStr + mText.mLen) {
        if (*p == '\n') {
            EmitLineGlyphs(flLineY, flLineWidth, nCharBase, pLine, p);
            flLineY += 1.0f;
            nCharBase += static_cast<int>(p - pLine);
            flLineWidth = 0.0f;
            pLine = p + 1;
            p = pLine;
            continue;
        }
        flLineWidth += mFont->GetCharAdvance(*p) + mFont->mSpace;
        ++p;
    }
    EmitLineGlyphs(flLineY, flLineWidth, nCharBase, pLine, p);

    mMesh->SyncAll();
    mMesh->Sync();
    mMesh->UpdateWorldXfm(this, 1);
}

// 0x004cf800
Text *NewText(const HxStr &name) {
    return new Text(name);
}

// 0x006feca8
Text *(*g_pfnNewText)(const HxStr &name) = NewText;

// 0x004cf1d0
Text *NewTextThroughHook(const HxStr &name) {
    return g_pfnNewText(name);
}

// 0x004cf770
Object *CreateRegisteredText(const HxStr &name) {
    return g_pfnNewText(name);
}

// 0x004cf190
void RegisterTextClass() {
    g_pfnNewText = NewText;
    g_manager.RegisterClass(g_textClassName, CreateRegisteredText);
}

// 0x006feca0
HxStr g_textClassName("Text");

} // namespace Rnd
