#include "rnd/transformable.h"

#include <algorithm>
#include <iterator>
#include <list>
#include <string.h>

#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/object.h"
#include "rnd/stream.h"

namespace Rnd {

// The only revision this build writes, and the highest it accepts.
constexpr int kTransformableRevision = 5;

// The first revision that stores mBillboard and mOrigin at all.
constexpr int kFirstRevisionWithOrigin = 1;

// The first revision that stores mBillboard as the mode word rather than as a legacy index.
constexpr int kFirstRevisionWithBillboardMode = 3;

// The revision range that stores one extra byte the reader discards.
constexpr int kFirstRevisionWithSpareByte = 2;
constexpr int kLastRevisionWithSpareByte = 4;

// Floats of a transform row that reach a stream or a sink. The fourth is padding and is skipped.
constexpr int kXfmRowStoredFloatCount = 3;

constexpr char kAlreadyInFormat[] = "%s already in %s\n";
constexpr char kCountFormat[] = "%d";
constexpr char kSizeFormat[] = "%u";
constexpr char kFloatFormat[] = "%.2f";
constexpr char kQuotedTextFormat[] = "\"%s\"";

// An empty HxStr stores a null buffer, and the binary substitutes the program-wide empty-string
// pointer at 0x006fbd10 rather than passing null to the formatter.
static const char *NameText(const Object *pObject) {
    return pObject->mName.mStr != nullptr ? pObject->mName.mStr : "";
}

// 0x004f8530
static FailSink &operator<<(FailSink &sink, const std::list<Transformable *> &transList) {
    sink.Print("(size:");
    sink.Format(kSizeFormat, transList.size());
    sink.Print(")");

    int nIndex = 0;
    for (std::list<Transformable *>::const_iterator it = transList.begin(); it != transList.end();
         ++it) {
        sink.Print("\n");
        sink.Format(kCountFormat, nIndex);
        sink.Print("\t");
        if (*it != nullptr) {
            sink.Format(kQuotedTextFormat, NameText(*it));
        } else {
            sink.Print("no object");
        }
        ++nIndex;
    }
    return sink;
}

// 0x004f26f0
// Only Transformable::DumpText() invokes this. A mode outside the set below writes nothing rather
// than a fallback title.
static FailSink &operator<<(FailSink &sink, Transformable::Billboard nBillboard) {
    switch (nBillboard) {
    case Transformable::kBillboardNone:
        sink.Print("None");
        break;
    case Transformable::kBillboardX:
        sink.Print("X");
        break;
    case Transformable::kBillboardY:
        sink.Print("Y");
        break;
    case Transformable::kBillboardZ:
        sink.Print("Z");
        break;
    case Transformable::kBillboardXZ:
        sink.Print("XZ");
        break;
    case Transformable::kBillboardXYZ:
        sink.Print("XYZ");
        break;
    case Transformable::kBillboardSimpleXYZ:
        sink.Print("SimpleXYZ");
        break;
    case Transformable::kBillboardLocalRotate:
        sink.Print("LocalRotate");
        break;
    case Transformable::kBillboardScaleX:
        sink.Print("ScaleX");
        break;
    case Transformable::kBillboardScaleY:
        sink.Print("ScaleY");
        break;
    case Transformable::kBillboardScaleZ:
        sink.Print("ScaleZ");
        break;
    case Transformable::kBillboardScaleXZ:
        sink.Print("ScaleXZ");
        break;
    case Transformable::kBillboardScaleXYZ:
        sink.Print("ScaleXYZ");
        break;
    case Transformable::kBillboardScaleSimpleXYZ:
        sink.Print("ScaleSimpleXYZ");
        break;
    }
    return sink;
}

// De-inlined from the nine places DumpText() repeats it. The padding word of a row is not
// written, and the origin uses the same shape with a different title in front of it.
static inline FailSink &DumpXfmRow(FailSink &sink, const float *pRow) {
    sink.Print("(x:");
    sink.Format(kFloatFormat, pRow[0]);
    sink.Print(" y:");
    sink.Format(kFloatFormat, pRow[1]);
    sink.Print(" z:");
    sink.Format(kFloatFormat, pRow[2]);
    sink.Print(")");
    return sink;
}

// 0x004f88a0
//
// Each entry is written as the referenced object's name including its terminator. A reader has to
// resolve the names through Rnd::g_manager. An empty entry writes one zero byte.
static Stream &operator<<(Stream &stream, const std::list<Transformable *> &transList) {
    int nCount = transList.size();
    stream.Write(&nCount, sizeof(nCount));

    for (std::list<Transformable *>::const_iterator it = transList.begin(); it != transList.end();
         ++it) {
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

// 0x004f8b78
static Stream &operator>>(Stream &stream, std::list<Transformable *> &transList) {
    int nCount = 0;
    stream.Read(&nCount, sizeof(nCount));
    transList.resize(nCount, nullptr);

    for (std::list<Transformable *>::iterator it = transList.begin(); it != transList.end(); ++it) {
        HxStr name(nullptr);
        stream.ReadString(name);
        Object *pObject = g_manager.Find(name);
        *it = dynamic_cast<Transformable *>(pObject);
    }
    return stream;
}

// Inlined at 0x00482c20 in Rnd::Mesh::Replace.
void Transformable::AdoptXfmFrom(const Transformable &owner) {
    memcpy(mLocalXfm, owner.mWorldXfm, sizeof(mLocalXfm));
    mDirty = 1;
    UpdateWorldXfm(nullptr, 0);

    memcpy(mLocalXfm, owner.mLocalXfm, sizeof(mLocalXfm));
    mDirty = 1;
    SetBillboard(owner.mBillboard);
    SetOrigin(owner.mOrigin);
    UpdateWorldXfm(nullptr, 0);
}

// Rows of a transform that store the basis, the row that stores the translation, and the index
// of the padding float in each row.
constexpr int kXfmBasisRowCount = 3;
constexpr int kXfmTranslationRow = 3;
constexpr int kXfmPaddingFloat = 3;

// VU0's vf0, the translation row of an identity transform and the origin of a new transformable.
constexpr float kIdentityTranslation[] = {0.0f, 0.0f, 0.0f, 1.0f};

// Write the identity into the three basis rows without touching their padding floats, and vf0
// into the translation row.
static inline void SetIdentityXfm(float (&aflXfm)[kXfmRowCount][kXfmRowFloatCount]) {
    for (int nRow = 0; nRow < kXfmBasisRowCount; ++nRow) {
        for (int i = 0; i < kXfmPaddingFloat; ++i) {
            aflXfm[nRow][i] = (i == nRow) ? 1.0f : 0.0f;
        }
    }
    std::copy(std::begin(kIdentityTranslation),
              std::end(kIdentityTranslation),
              aflXfm[kXfmTranslationRow]);
}

// 0x004fb3f8
Transformable::Transformable() : mDirty(1), mBillboard(kBillboardNone) {
    for (auto &aflRow : mLocalXfm) {
        aflRow[kXfmPaddingFloat] = 1.0f;
    }
    for (auto &aflRow : mWorldXfm) {
        aflRow[kXfmPaddingFloat] = 1.0f;
    }
    SetIdentityXfm(mLocalXfm);
    SetIdentityXfm(mWorldXfm);
    std::copy(std::begin(kIdentityTranslation), std::end(kIdentityTranslation), mOrigin);
}

// 0x004fb2a8
Transformable::~Transformable() {
    ReleaseTransRefs();
}

// 0x004f0838
void Transformable::AddTrans(Transformable *pTrans) {
    if (std::find(mTransList.begin(), mTransList.end(), pTrans) != mTransList.end()) {
        g_failSink.Report(kAlreadyInFormat, NameText(pTrans), NameText(this));
        return;
    }

    if (pTrans != nullptr) {
        pTrans->AddRef(this);
    }
    mTransList.push_back(pTrans);
    pTrans->mDirty = 1; // Yes, the binary dereferences pTrans here with no null test.
}

// 0x004f0a80
void Transformable::ClearTransList() {
    for (std::list<Transformable *>::iterator it = mTransList.begin(); it != mTransList.end();) {
        if (*it != nullptr) {
            (*it)->RemoveRef(this);
        }
        it = mTransList.erase(it);
    }
}

// 0x004fcf18
void Transformable::ReleaseTransRefs() {
    for (std::list<Transformable *>::iterator it = mTransList.begin(); it != mTransList.end();
         ++it) {
        if (*it != nullptr) {
            (*it)->RemoveRef(this);
        }
    }
}

// 0x004fcf88
void Transformable::AcquireTransRefs() {
    mDirty = 1;
    for (std::list<Transformable *>::iterator it = mTransList.begin(); it != mTransList.end();
         ++it) {
        if (*it != nullptr) {
            (*it)->AddRef(this);
        }
    }
}

// 0x004f12e0
void Transformable::DumpText(FailSink &sink) {
    if (sink.mDumpLevel <= 0) {
        return;
    }
    sink.Print("[Transformable]\n");

    sink.Print("localXfm:");
    for (int nRow = 0; nRow < kXfmRowCount; ++nRow) {
        sink.Print("\n\t");
        DumpXfmRow(sink, mLocalXfm[nRow]);
    }
    sink.Print("\n");

    sink.Print("worldXfm:");
    for (int nRow = 0; nRow < kXfmRowCount; ++nRow) {
        sink.Print("\n\t");
        DumpXfmRow(sink, mWorldXfm[nRow]);
    }
    sink.Print("\n");

    sink.Print("transList:");
    sink << mTransList;
    sink.Print("\n");

    sink.Print("billboard:");
    sink << static_cast<Billboard>(mBillboard);
    sink.Print(" origin:");
    DumpXfmRow(sink, mOrigin);
    sink.Print("\n");
}

// 0x004f1a58
void Transformable::Save(Stream &stream) {
    int nRevision = kTransformableRevision;
    stream.Write(&nRevision, sizeof(nRevision));

    for (int nRow = 0; nRow < kXfmRowCount; ++nRow) {
        for (int nAxis = 0; nAxis < kXfmRowStoredFloatCount; ++nAxis) {
            float flValue = mLocalXfm[nRow][nAxis];
            stream.Write(&flValue, sizeof(flValue));
        }
    }
    for (int nRow = 0; nRow < kXfmRowCount; ++nRow) {
        for (int nAxis = 0; nAxis < kXfmRowStoredFloatCount; ++nAxis) {
            float flValue = mWorldXfm[nRow][nAxis];
            stream.Write(&flValue, sizeof(flValue));
        }
    }

    stream << mTransList;

    int nBillboard = mBillboard;
    stream.Write(&nBillboard, sizeof(nBillboard));

    for (int nAxis = 0; nAxis < kXfmRowStoredFloatCount; ++nAxis) {
        float flValue = mOrigin[nAxis];
        stream.Write(&flValue, sizeof(flValue));
    }
}

// 0x004f1f18
void Transformable::Load(Stream &stream) {
    int nRevision = 0;
    stream.Read(&nRevision, sizeof(nRevision));
    if (nRevision > kTransformableRevision) {
        g_failSink.Report("Can't load new Transformable\n");
        if (g_failSink.mAbortProc != nullptr) {
            g_failSink.mAbortProc();
        }
    }

    ReleaseTransRefs();

    for (int nRow = 0; nRow < kXfmRowCount; ++nRow) {
        for (int nAxis = 0; nAxis < kXfmRowStoredFloatCount; ++nAxis) {
            stream.Read(&mLocalXfm[nRow][nAxis], sizeof(mLocalXfm[nRow][nAxis]));
        }
    }
    for (int nRow = 0; nRow < kXfmRowCount; ++nRow) {
        for (int nAxis = 0; nAxis < kXfmRowStoredFloatCount; ++nAxis) {
            stream.Read(&mWorldXfm[nRow][nAxis], sizeof(mWorldXfm[nRow][nAxis]));
        }
    }

    stream >> mTransList;

    if (nRevision >= kFirstRevisionWithOrigin) {
        if (nRevision < kFirstRevisionWithBillboardMode) {
            // The bound below is the size of the table in bytes rather than its element count, so
            // a legacy index from 6 to 23 reads past the end. That is what the binary does.
            const int anLegacyBillboard[] = {
                kBillboardNone, kBillboardX, kBillboardY, kBillboardZ, kBillboardXZ, kBillboardXYZ};
            int nLegacy = 0;
            stream.Read(&nLegacy, sizeof(nLegacy));
            mBillboard = static_cast<unsigned>(nLegacy) < sizeof(anLegacyBillboard) ?
                             anLegacyBillboard[nLegacy] :
                             kBillboardNone;
        } else {
            stream.Read(&mBillboard, sizeof(mBillboard));
        }

        for (int nAxis = 0; nAxis < kXfmRowStoredFloatCount; ++nAxis) {
            stream.Read(&mOrigin[nAxis], sizeof(mOrigin[nAxis]));
        }
    }

    if (nRevision >= kFirstRevisionWithSpareByte && nRevision <= kLastRevisionWithSpareByte) {
        char cSpare = 0;
        stream.ReadBytes(&cSpare, sizeof(cSpare)); // Read and then discarded, as in the binary.
    }

    AcquireTransRefs();
}

// 0x004f2510
void Transformable::Replace(Object *pFrom, Object *pTo) {
    for (std::list<Transformable *>::iterator it = mTransList.begin(); it != mTransList.end();) {
        if (*it == pTo) {
            g_failSink.Report(kAlreadyInFormat, NameText(pTo), NameText(this));
        }

        if (*it == pFrom) {
            if (pFrom != nullptr) {
                pFrom->RemoveRef(this);
            }
            if (*it != nullptr) {
                *it = dynamic_cast<Transformable *>(pTo);
            }
            if (*it != nullptr) {
                (*it)->AddRef(this);
            }
        }

        if (*it == nullptr) {
            it = mTransList.erase(it);
        } else {
            ++it;
        }
    }
}

// 0x004fce30
void Transformable::Copy(const Object *pSource, unsigned nFlags) {
    const Transformable *pSourceTrans = dynamic_cast<const Transformable *>(pSource);

    ReleaseTransRefs();

    memcpy(mLocalXfm, pSourceTrans->mLocalXfm, sizeof(mLocalXfm));
    memcpy(mWorldXfm, pSourceTrans->mWorldXfm, sizeof(mWorldXfm));
    mBillboard = pSourceTrans->mBillboard;
    memcpy(mOrigin, pSourceTrans->mOrigin, sizeof(mOrigin));

    if ((nFlags & kCopyChildLists) != 0) {
        mTransList = pSourceTrans->mTransList;
    }
    AcquireTransRefs();
}

} // namespace Rnd
