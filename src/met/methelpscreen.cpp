#include "met/methelpscreen.h"

#include <cstring>

#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/font.h"
#include "rnd/manager.h"
#include "rnd/text.h"
#include "rnd/view.h"
#include "script/cxx/list.h"
#include "script/cxx/seqbase.h"
#include "script/cxx/string.h"
#include "script/cxx/tuple.h"
#include "script/scripteval.h"

namespace {

static const char *const kScreenName = "so";
static const char *const kDirectory = "metagame/shared";
static const char *const kContainerName = "options_sl";
static const char *const kRegisteredName = "MetHelpScreen";

// Counted from 1.
static const char *const kInfoTextFormat = "sl_pan_opt_info_0%d.txt";
static const char *const kTitleTextFormat = "sl_opt_title_0%d.txt";

static const char *const kShowAnimation = "so_TT_01.anim";
static const char *const kHideAnimation = "so_TT_02.anim";
static const char *const kNoText = "";

constexpr int kInfoTextCount = 6;
constexpr int kTitleTextCount = 4;

// The script template that maps a prompt or a layout to its list of (font, text) entries.
constexpr int kHelpTextTemplate = 616;

constexpr int kEntryFieldFont = 0;
constexpr int kEntryFieldText = 1;

constexpr int kTranslationRow = 3;
constexpr float kVectorPadding = 1.0f;
constexpr float kHalf = 0.5f;

constexpr int kFillInfoTexts = 0;
constexpr int kFillTitleTexts = 1;

inline Rnd::Text *FindText(const char *pszName) {
    return dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(HxStr(pszName)));
}

inline Rnd::Animatable *FindAnimation(const char *pszName) {
    return dynamic_cast<Rnd::Animatable *>(Rnd::g_manager.Find(HxStr(pszName)));
}

inline MetHelpScreen *FindHelpScreen() {
    return dynamic_cast<MetHelpScreen *>(MetScreen::FindScreenByName(HxStr(kRegisteredName)));
}

} // namespace

// 0x003125b0
MetHelpScreen::MetHelpScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mUnknown8c(0.0f), mUnknown90(0.0f) {
    mUnknownb0.w = kVectorPadding;
    mUnknownd0.w = kVectorPadding;
    mUnknown60 = 0;
}

// 0x00312770
MetHelpScreen::~MetHelpScreen() {
}

// 0x003128d0
void MetHelpScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();

    for (int i = 0; i < kInfoTextCount; ++i) {
        Rnd::Text *pText = FindText(FormatString(kInfoTextFormat, i + 1));
        pText->SetText(HxStr(kNoText));
        pText->SetShowing(1);
        mUnknowna4.push_back(pText);
    }
    std::memcpy(&mUnknownb0, mUnknowna4[0]->mLocalXfm[kTranslationRow], sizeof(mUnknownb0));

    for (int i = 0; i < kTitleTextCount; ++i) {
        Rnd::Text *pText = FindText(FormatString(kTitleTextFormat, i + 1));
        pText->SetText(HxStr(kNoText));
        pText->SetShowing(1);
        mUnknownc0.push_back(pText);
    }
    std::memcpy(&mUnknownd0, mUnknownc0[0]->mLocalXfm[kTranslationRow], sizeof(mUnknownd0));

    mUnknowne8 = FindAnimation(kShowAnimation);
    mUnknownf0 = FindAnimation(kHideAnimation);
    if (mUnknowne8 != nullptr) {
        mUnknownec = mUnknowne8->EndFrame();
    }
    if (mUnknownf0 != nullptr) {
        mUnknownf4 = mUnknownf0->EndFrame();
    }
}

// 0x00312df0
void MetHelpScreen::OnUnknownSlot26(float flTime) {
    UpdateShow(flTime);
    UpdateHide(flTime);
}

// 0x00312f70
void MetHelpScreen::PostText(const HxStr &text, float flTime) {
    if (mUnknown94 == kNoText && text == kNoText) {
        return;
    }
    const bool bAnimating = mUnknown8c != 0.0f || mUnknown90 != 0.0f;
    if (!bAnimating && text == mUnknown94) {
        return;
    }
    if (mUnknown48 != 0) {
        return;
    }

    if (mUnknown94 == kNoText) {
        mUnknown94 = text;
        StartShow(flTime);
    } else {
        StartHide(flTime);
        mUnknown9c = text;
    }
}

// 0x003130e8
void MetHelpScreen::ClearInfoTexts() {
    for (std::vector<Rnd::Text *>::size_type i = 0; i < mUnknowna4.size(); ++i) {
        mUnknowna4[i]->SetText(HxStr(kNoText));
    }
    mUnknown14->UpdateWorldXfm(nullptr, 1); // Yes, the binary discards the result.
}

// 0x00313208
void MetHelpScreen::FillTexts(const HxStr &key,
                              std::vector<Rnd::Text *> &texts,
                              const Vector3 &origin,
                              int nTitles) {
    Py::Object result =
        EvalScriptTemplate(kHelpTextTemplate, key.mStr != nullptr ? key.mStr : g_szEmptyString);
    if (result.isList()) {
        Py::List entries(result);
        const int nCount = entries.length();
        Vector3 end = origin;
        for (std::vector<Rnd::Text *>::size_type i = 0; i < texts.size(); ++i) {
            texts[i]->SetText(HxStr(kNoText));
        }

        for (int i = 0; i < nCount; ++i) {
            if (entries[i].isTuple()) {
                Py::Tuple entry(entries[i]);
                Py::String font(entry[kEntryFieldFont]);
                Py::String value(entry[kEntryFieldText]);
                HxStr fontName = font.as_string();
                HxStr textValue = value.as_string();
                texts[i]->mDirty = 1;
                texts[i]->SetFont(dynamic_cast<Rnd::Font *>(Rnd::g_manager.Find(fontName)));
                texts[i]->SetText(textValue);
                Vector3 advance = texts[i]->CharPosition(textValue.mLen);
                AddVec3(&end.x, &advance.x, &end.x);
            }
        }

        const Vector3 shift{(end.x - origin.x) * kHalf, 0.0f, 0.0f, kVectorPadding};
        for (int i = 0; i < nCount; ++i) {
            Vector3 translation;
            std::memcpy(&translation, texts[i]->mLocalXfm[kTranslationRow], sizeof(translation));
            Vector3 centred;
            centred.w = kVectorPadding;
            Vec3Sub(&translation.x, &shift.x, &centred.x);
            std::memcpy(texts[i]->mLocalXfm[kTranslationRow], &centred, sizeof(centred));
            texts[i]->mDirty = 1;
        }
    }

    if (nTitles == kFillInfoTexts) {
        mUnknown14->UpdateWorldXfm(nullptr, 1); // Yes, the binary discards the result.
    }
}

// 0x00317210
MetHelpScreen *MetHelpScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetHelpScreen(pRenderer, nPriority);
}

// 0x00317298
void MetHelpScreen::SelectPreset(const HxStr &name) {
    FindHelpScreen()->ApplyPreset(name);
}

// 0x00317368
void MetHelpScreen::SetText(const HxStr &text, float flTime) {
    FindHelpScreen()->PostText(text, flTime);
}

// 0x00317448
void MetHelpScreen::HandleCommand(const MetScreenCommand *pCommand) {
    if (pCommand->mCommand == kMetScreenCommandBack) {
        BeginExit();
    }
}

// 0x00317480
void MetHelpScreen::OnUnknownSlot36() {
    ClearInfoTexts();
    mUnknown94 = kNoText;
    mUnknown9c = kNoText;
    mUnknown90 = 0.0f;
}

// 0x00317508
void MetHelpScreen::StartShow(float flTime) {
    if (mUnknown8c == 0.0f) {
        mUnknown8c = flTime;
        FillTexts(mUnknown94, mUnknowna4, mUnknownb0, kFillInfoTexts);
        mUnknowne8->SetFrame(0.0f);
    }
}

// 0x00317568
void MetHelpScreen::UpdateShow(float flTime) {
    if (mUnknown8c == 0.0f) {
        return;
    }
    if (mUnknowne8 != nullptr) {
        mUnknowne8->SetFrame(flTime - mUnknown8c);
    }
    if (mUnknown8c + mUnknownec < flTime) {
        mUnknown8c = 0.0f;
        if (mUnknown9c != kNoText) {
            StartHide(flTime);
        }
    }
}

// 0x00317600
void MetHelpScreen::StartHide(float flTime) {
    if (mUnknown90 == 0.0f) {
        mUnknown90 = flTime;
        mUnknownf0->SetFrame(0.0f);
    }
}

// 0x00317640
void MetHelpScreen::UpdateHide(float flTime) {
    if (mUnknown90 == 0.0f) {
        return;
    }
    if (mUnknownf0 != nullptr) {
        mUnknownf0->SetFrame(flTime - mUnknown90);
    }
    if (mUnknown90 + mUnknownf4 < flTime) {
        mUnknown90 = 0.0f;
        mUnknown94 = mUnknown9c;
        mUnknown9c = kNoText;
        if (mUnknown94 != kNoText) {
            StartShow(flTime);
        } else {
            ClearInfoTexts();
        }
    }
}

// 0x00317758
void MetHelpScreen::ApplyPreset(const HxStr &name) {
    mUnknowne0 = name;
    if (mUnknown48 == 0) {
        FillTexts(name, mUnknownc0, mUnknownd0, kFillTitleTexts);
    }
}
