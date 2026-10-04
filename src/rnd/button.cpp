#include "rnd/button.h"

#include <vector>

#include "os/dbg.h"
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

void PrintObjectRef(Dbg &sink, const Object *pObject) {
    if (pObject == nullptr) {
        sink.Print(kNoObject);
        return;
    }
    sink.Format("\"%s\"", NameText(pObject));
}

void WriteObjectRef(Stream &stream, const Object *pObject) {
    if (pObject == nullptr) {
        const char chTerminator = '\0';
        stream.Write(&chTerminator, 1);
        return;
    }
    stream.Write(NameText(pObject), pObject->mName.mLen + 1);
}

// The three operators below are instantiated twice each, once for the material palette and once
// for the font palette, and every instantiation is called only from this class. They read the name
// through the Rnd::Object subobject, which costs nothing because both element classes derive from
// Rnd::Object non-virtually at offset 0.

// Dump a palette as its size and one indexed name per entry.
template <class T>
Dbg &operator<<(Dbg &sink, const std::vector<T *> &entries) {
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
    stream.WriteLE(&nCount, sizeof(nCount));

    for (unsigned i = 0; i < entries.size(); ++i) {
        WriteObjectRef(stream, entries[i]);
    }
    return stream;
}

// Read a palette written by the writer above, resolving each name through Rnd::TheManager.
template <class T>
Stream &operator>>(Stream &stream, std::vector<T *> &entries) {
    int nCount = 0;
    stream.ReadLE(&nCount, sizeof(nCount));
    entries.resize(nCount, static_cast<T *>(nullptr));

    for (unsigned i = 0; i < entries.size(); ++i) {
        HxStr name(nullptr);
        stream.ReadString(name);
        entries[i] = dynamic_cast<T *>(TheManager.Find(name));
    }
    return stream;
}

// NTSC-U/C: 0x005336b0, PAL: 0x00572ed0
template Dbg &operator<< <Mat>(Dbg &sink, const std::vector<Mat *> &entries);

// NTSC-U/C: 0x005337e8, PAL: 0x00573008
template Dbg &operator<< <Font>(Dbg &sink, const std::vector<Font *> &entries);

// NTSC-U/C: 0x00533920, PAL: 0x00573140
template Stream &operator<< <Mat>(Stream &stream, const std::vector<Mat *> &entries);

// NTSC-U/C: 0x00533a10, PAL: 0x00573230
template Stream &operator<< <Font>(Stream &stream, const std::vector<Font *> &entries);

// NTSC-U/C: 0x00533e08, PAL: 0x00573628
template Stream &operator>> <Mat>(Stream &stream, std::vector<Mat *> &entries);

// NTSC-U/C: 0x00534290, PAL: 0x00573ad8
template Stream &operator>> <Font>(Stream &stream, std::vector<Font *> &entries);

} // namespace

Button::Button(const HxStr &name)
    : Object(name), mState(0), mMesh(nullptr), mText(nullptr),
      mMats(kInitialPaletteSize, static_cast<Mat *>(nullptr)),
      mFonts(kInitialPaletteSize, static_cast<Font *>(nullptr)) {
}

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

Button::~Button() {
    RemoveObjectRefs();
    ReleaseAllRefs();
}

const HxStr &Button::ClassName() const {
    return g_buttonClassName;
}

void Button::DumpText(Dbg &sink) {
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

void Button::Save(Stream &stream) {
    const int nVersion = kSerialVersion;
    stream.WriteLE(&nVersion, sizeof(nVersion));
    stream.WriteLE(&mState, sizeof(mState));
    WriteObjectRef(stream, mMesh);
    WriteObjectRef(stream, mText);
    stream << mMats;
    stream << mFonts;
}

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

void Button::Load(Stream &stream) {
    stream.ReadLE(&g_nRndButtonLoadVersion, sizeof(g_nRndButtonLoadVersion));
    if (g_nRndButtonLoadVersion > kSerialVersion) {
        Rnd::TheDbg.Notify("Can't load new Button\n");
        return;
    }

    RemoveObjectRefs();

    int nState = 0;
    stream.ReadLE(&nState, sizeof(nState));
    mState = nState;

    HxStr meshName(nullptr);
    stream.ReadString(meshName);
    mMesh = dynamic_cast<Mesh *>(TheManager.Find(meshName));

    HxStr textName(nullptr);
    stream.ReadString(textName);
    mText = dynamic_cast<Text *>(TheManager.Find(textName));

    stream >> mMats;
    stream >> mFonts;

    AddObjectRefs();
}

void *Button::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, kButtonTag);
}

void Button::operator delete(void *pBlock) {
    OperatorDeleteOverride(pBlock, kButtonTag);
}

void Button::SetShowing(int nShowing) {
    if (mMesh != nullptr) {
        mMesh->SetShowing(nShowing);
    }
    if (mText != nullptr) {
        mText->SetShowing(nShowing);
    }
}

void Button::SetState(int nState) {
    if (mState == nState) {
        return;
    }
    mState = nState;
    if (mMesh != nullptr) {
        mMesh->SetMat(mMats[nState]);
    }
    if (mText != nullptr) {
        mText->SetFont(mFonts[nState]);
    }
}

void Button::SetMesh(Mesh *pMesh) {
    if (mMesh != nullptr) {
        mMesh->RemoveRef(this);
    }
    mMesh = pMesh;
    if (pMesh != nullptr) {
        pMesh->AddRef(this);
    }
    if (pMesh != nullptr) {
        pMesh->SetMat(mMats[mState]);
    }
}

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

void Button::SetMat(int nState, Mat *pMat) {
    if (mMats[nState] != nullptr) {
        mMats[nState]->RemoveRef(this);
    }
    mMats[nState] = pMat;
    if (mMats[nState] != nullptr) {
        mMats[nState]->AddRef(this);
    }
    if (mMesh != nullptr && mState == nState) {
        mMesh->SetMat(pMat);
    }
}

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

Button *NewButton(const HxStr &name) {
    return new Button(name);
}

// NTSC-U/C: 0x0071d900, PAL: 0x00761370
Button *(*g_pfnNewButton)(const HxStr &name) = NewButton;

Button *NewButtonThroughHook(const HxStr &name) {
    try {
        return g_pfnNewButton(name);
    } catch (...) {
        return nullptr; // The binary's handler returns null.
    }
}

Object *CreateRegisteredButton(const HxStr &name) {
    try {
        return g_pfnNewButton(name);
    } catch (...) {
        return nullptr;
    }
}

void RegisterButtonClass() {
    g_pfnNewButton = NewButton;
    TheManager.RegisterClass(g_buttonClassName, CreateRegisteredButton);
}

// NTSC-U/C: 0x0071d8f8, PAL: 0x00761368
HxStr g_buttonClassName("Button");

// NTSC-U/C: 0x0089e060, PAL: 0x008e2ffc
int g_nRndButtonLoadVersion;

} // namespace Rnd
