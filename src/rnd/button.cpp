#include "rnd/button.h"

#include <vector>

#include "os/failsink.h"
#include "os/hxstr.h"
#include "os/mem.h"
#include "rnd/font.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"
#include "rnd/object.h"
#include "rnd/stream.h"
#include "rnd/text.h"

namespace Rnd {

namespace {

const char *const kButtonTag = "Rnd::Button";

constexpr int kSerialVersion = 0;
constexpr char kNoObject[] = "no object";

/** Entries each palette starts with, every one of them null. */
constexpr int kInitialPaletteSize = 1;

const char *NameText(const Object *pObject) {
    return pObject->mName.mStr != nullptr ? pObject->mName.mStr : "";
}

void PrintObjectRef(FailSink &sink, const Object *pObject) {
    if (pObject == nullptr) {
        sink.Print(kNoObject);
        return;
    }
    sink.Format("\"%s\"", NameText(pObject));
}

void WriteObjectRef(Stream &stream, const Object *pObject) {
    if (pObject == nullptr) {
        const char chTerminator = '\0';
        stream.WriteBytes(&chTerminator, 1);
        return;
    }
    stream.WriteBytes(NameText(pObject), pObject->mName.mLen + 1);
}

// The three operators below are instantiated twice each, once for the material palette and once
// for the font palette, and every instantiation is called only from this class. They read the name
// through the Rnd::Object subobject, which costs nothing because both element classes derive from
// Rnd::Object non-virtually at offset 0.

// Dump a palette as its size and one indexed name per entry.
template <class T>
FailSink &operator<<(FailSink &sink, const std::vector<T *> &entries) {
    sink.Print("(size:");
    sink.Format("%u", entries.size());
    sink.Print(")");

    for (unsigned i = 0; i < entries.size(); ++i) {
        sink.Print("\n");
        sink.Format("%d", i);
        sink.Print("\t");
        PrintObjectRef(sink, entries[i]);
    }
    return sink;
}

// Write a palette as its count and one name per entry.
template <class T>
Stream &operator<<(Stream &stream, const std::vector<T *> &entries) {
    const int nCount = entries.size();
    stream.Write(&nCount, sizeof(nCount));

    for (unsigned i = 0; i < entries.size(); ++i) {
        WriteObjectRef(stream, entries[i]);
    }
    return stream;
}

// Read a palette written by the writer above, resolving each name through Rnd::g_manager.
template <class T>
Stream &operator>>(Stream &stream, std::vector<T *> &entries) {
    int nCount = 0;
    stream.Read(&nCount, sizeof(nCount));
    entries.resize(nCount, static_cast<T *>(nullptr));

    for (unsigned i = 0; i < entries.size(); ++i) {
        HxStr name(nullptr);
        stream.ReadString(name);
        entries[i] = dynamic_cast<T *>(g_manager.Find(name));
    }
    return stream;
}

// 0x005336b0
template FailSink &operator<< <Mat>(FailSink &sink, const std::vector<Mat *> &entries);

// 0x005337e8
template FailSink &operator<< <Font>(FailSink &sink, const std::vector<Font *> &entries);

// 0x00533920
template Stream &operator<< <Mat>(Stream &stream, const std::vector<Mat *> &entries);

// 0x00533a10
template Stream &operator<< <Font>(Stream &stream, const std::vector<Font *> &entries);

// 0x00533e08
template Stream &operator>> <Mat>(Stream &stream, std::vector<Mat *> &entries);

// 0x00534290
template Stream &operator>> <Font>(Stream &stream, std::vector<Font *> &entries);

} // namespace

// 0x00530eb8
Button::Button(const HxStr &name)
    : Object(name), mState(0), mMesh(nullptr), mText(nullptr),
      mMats(kInitialPaletteSize, static_cast<Mat *>(nullptr)),
      mFonts(kInitialPaletteSize, static_cast<Font *>(nullptr)) {
}

// 0x00534900
void Button::RemoveObjectRefs() {
    if (mMesh != nullptr) {
        mMesh->RemoveRef(this);
    }
    if (mText != nullptr) {
        mText->RemoveRef(this);
    }
    for (unsigned i = 0; i < mMats.size(); ++i) {
        if (mMats[i] != nullptr) {
            mMats[i]->RemoveRef(this);
        }
    }
    for (unsigned i = 0; i < mFonts.size(); ++i) {
        if (mFonts[i] != nullptr) {
            mFonts[i]->RemoveRef(this);
        }
    }
}

// 0x00534820
void Button::AddObjectRefs() {
    if (mMesh != nullptr) {
        mMesh->AddRef(this);
    }
    if (mText != nullptr) {
        mText->AddRef(this);
    }
    for (unsigned i = 0; i < mMats.size(); ++i) {
        if (mMats[i] != nullptr) {
            mMats[i]->AddRef(this);
        }
    }
    for (unsigned i = 0; i < mFonts.size(); ++i) {
        if (mFonts[i] != nullptr) {
            mFonts[i]->AddRef(this);
        }
    }
}

// 0x00530cd0
Button::~Button() {
    RemoveObjectRefs();
    ReleaseAllRefs();
}

// 0x00534620
const HxStr &Button::ClassName() const {
    return g_buttonClassName;
}

// 0x005304f0
void Button::DumpText(FailSink &sink) {
    Object::DumpText(sink);
    if (sink.mDumpLevel <= 0) {
        return;
    }

    sink.Print("[Button]\n");
    sink.Print("state: ");
    sink.Format("%d", mState);
    sink.Print(" mesh: ");
    PrintObjectRef(sink, mMesh);
    sink.Print(" text: ");
    PrintObjectRef(sink, mText);
    sink.Print("\n");
    sink.Print("mats: ");
    sink << mMats;
    sink.Print("\n");
    sink.Print("fonts: ");
    sink << mFonts;
    sink.Print("\n");
}

// 0x00530690
void Button::Save(Stream &stream) {
    const int nVersion = kSerialVersion;
    stream.Write(&nVersion, sizeof(nVersion));
    stream.Write(&mState, sizeof(mState));
    WriteObjectRef(stream, mMesh);
    WriteObjectRef(stream, mText);
    stream << mMats;
    stream << mFonts;
}

// 0x00530a20
void Button::Replace(Object *pFrom, Object *pTo) {
    if (mMesh == pFrom) {
        if (pFrom != nullptr) {
            pFrom->RemoveRef(this);
        }
        if (mMesh != nullptr) {
            mMesh = pTo != nullptr ? dynamic_cast<Mesh *>(pTo) : nullptr;
        }
        if (mMesh != nullptr) {
            mMesh->AddRef(this);
        }
    }

    if (mText == pFrom) {
        if (pFrom != nullptr) {
            pFrom->RemoveRef(this);
        }
        if (mText != nullptr) {
            mText = pTo != nullptr ? dynamic_cast<Text *>(pTo) : nullptr;
        }
        if (mText != nullptr) {
            mText->AddRef(this);
        }
    }

    for (unsigned i = 0; i < mMats.size(); ++i) {
        if (mMats[i] != pFrom) {
            continue;
        }
        if (pFrom != nullptr) {
            pFrom->RemoveRef(this);
        }
        if (mMats[i] == nullptr) {
            continue;
        }
        mMats[i] = pTo != nullptr ? dynamic_cast<Mat *>(pTo) : nullptr;
        if (mMats[i] != nullptr) {
            mMats[i]->AddRef(this);
        }
    }

    for (unsigned i = 0; i < mFonts.size(); ++i) {
        if (mFonts[i] != pFrom) {
            continue;
        }
        if (pFrom != nullptr) {
            pFrom->RemoveRef(this);
        }
        if (mFonts[i] == nullptr) {
            continue;
        }
        mFonts[i] = pTo != nullptr ? dynamic_cast<Font *>(pTo) : nullptr;
        if (mFonts[i] != nullptr) {
            mFonts[i]->AddRef(this);
        }
    }
}

// 0x00534780
void Button::Copy(const Object *pSource, [[maybe_unused]] unsigned nFlags) {
    // The cast result is dereferenced with no null check, so a pSource of another class faults here
    // rather than being rejected. No base implementation is invoked and nFlags is never read.
    const Button *pButton = pSource != nullptr ? dynamic_cast<const Button *>(pSource) : nullptr;

    RemoveObjectRefs();
    mMesh = nullptr;
    mText = nullptr;
    mState = pButton->mState;
    mMats = pButton->mMats;
    mFonts = pButton->mFonts;
    AddObjectRefs();
}

// 0x005307f8
void Button::Load(Stream &stream) {
    stream.Read(&g_nRndButtonLoadVersion, sizeof(g_nRndButtonLoadVersion));
    if (g_nRndButtonLoadVersion > kSerialVersion) {
        // The report is the whole of the response. No other renderer class returns here without
        // transferring control to the abort handler of g_failSink.
        g_failSink.Report("Can't load new Button\n");
        return;
    }

    RemoveObjectRefs();

    int nState = 0;
    stream.Read(&nState, sizeof(nState));
    mState = nState;

    HxStr meshName(nullptr);
    stream.ReadString(meshName);
    mMesh = dynamic_cast<Mesh *>(g_manager.Find(meshName));

    HxStr textName(nullptr);
    stream.ReadString(textName);
    mText = dynamic_cast<Text *>(g_manager.Find(textName));

    stream >> mMats;
    stream >> mFonts;

    AddObjectRefs();
}

// 0x005344b8
void *Button::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, kButtonTag);
}

// 0x005344d8
void Button::operator delete(void *pBlock) {
    FreeTaggedMemory(pBlock, kButtonTag);
}

// 0x005349e0
void Button::SetShowing(int nShowing) {
    if (mMesh != nullptr) {
        mMesh->SetShowing(nShowing);
    }
    if (mText != nullptr) {
        mText->SetShowing(nShowing);
    }
}

// 0x00534a48
void Button::SetState(int nState) {
    if (mState == nState) {
        return;
    }
    mState = nState;
    if (mMesh != nullptr) {
        mMesh->SetMaterial(mMats[nState]);
    }
    if (mText != nullptr) {
        mText->SetFont(mFonts[nState]);
    }
}

// 0x00534ad0
void Button::SetMesh(Mesh *pMesh) {
    if (mMesh != nullptr) {
        mMesh->RemoveRef(this);
    }
    mMesh = pMesh;
    if (pMesh != nullptr) {
        pMesh->AddRef(this);
    }
    if (pMesh != nullptr) {
        pMesh->SetMaterial(mMats[mState]);
    }
}

// 0x00534b48
void Button::SetText(Text *pText) {
    if (mText != nullptr) {
        mText->RemoveRef(this);
    }
    mText = pText;
    if (pText != nullptr) {
        pText->AddRef(this);
    }
    if (pText != nullptr) {
        pText->SetFont(mFonts[mState]);
    }
}

// 0x00534bd0
void Button::SetMat(int nState, Mat *pMat) {
    if (mMats[nState] != nullptr) {
        mMats[nState]->RemoveRef(this);
    }
    mMats[nState] = pMat;
    if (mMats[nState] != nullptr) {
        mMats[nState]->AddRef(this);
    }
    if (mMesh != nullptr && mState == nState) {
        mMesh->SetMaterial(pMat);
    }
}

// 0x00534c78
void Button::SetFont(int nState, Font *pFont) {
    if (mFonts[nState] != nullptr) {
        mFonts[nState]->RemoveRef(this);
    }
    mFonts[nState] = pFont;
    if (mFonts[nState] != nullptr) {
        mFonts[nState]->AddRef(this);
    }
    if (mText != nullptr && mState == nState) {
        mText->SetFont(pFont);
    }
}

// 0x005346f8
Button *NewButton(const HxStr &name) {
    return new Button(name);
}

// 0x0071d900
Button *(*g_pfnNewButton)(const HxStr &name) = NewButton;

// 0x00534538
Button *NewButtonThroughHook(const HxStr &name) {
    try {
        return g_pfnNewButton(name);
    } catch (...) {
        return nullptr; // The binary's handler returns null.
    }
}

// 0x00534678
Object *CreateRegisteredButton(const HxStr &name) {
    try {
        return g_pfnNewButton(name);
    } catch (...) {
        return nullptr;
    }
}

// 0x005344f8
void RegisterButtonClass() {
    g_pfnNewButton = NewButton;
    g_manager.RegisterClass(g_buttonClassName, CreateRegisteredButton);
}

// 0x0071d8f8
HxStr g_buttonClassName("Button");

// 0x0089e060
int g_nRndButtonLoadVersion;

} // namespace Rnd
