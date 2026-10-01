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
      mShowStart(0.0f), mHideStart(0.0f) {
    mInfoOrigin.w = kVectorPadding;
    mTitleOrigin.w = kVectorPadding;
    mShowsLoadedDrawables = 0;
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
        mInfoTexts.push_back(pText);
    }
    std::memcpy(&mInfoOrigin, mInfoTexts[0]->mLocalXfm[kTranslationRow], sizeof(mInfoOrigin));

    for (int i = 0; i < kTitleTextCount; ++i) {
        Rnd::Text *pText = FindText(FormatString(kTitleTextFormat, i + 1));
        pText->SetText(HxStr(kNoText));
        pText->SetShowing(1);
        mTitleTexts.push_back(pText);
    }
    std::memcpy(&mTitleOrigin, mTitleTexts[0]->mLocalXfm[kTranslationRow], sizeof(mTitleOrigin));

    mShowAnim = FindAnimation(kShowAnimation);
    mHideAnim = FindAnimation(kHideAnimation);
    if (mShowAnim != nullptr) {
        mShowEnd = mShowAnim->EndFrame();
    }
    if (mHideAnim != nullptr) {
        mHideEnd = mHideAnim->EndFrame();
    }
}

// 0x00312df0
void MetHelpScreen::UpdateIdle(float flTime) {
    UpdateShow(flTime);
    UpdateHide(flTime);
}

// 0x00312f70
void MetHelpScreen::PostText(const HxStr &text, float flTime) {
    if (mShownText == kNoText && text == kNoText) {
        return;
    }
    const bool bAnimating = mShowStart != 0.0f || mHideStart != 0.0f;
    if (!bAnimating && text == mShownText) {
        return;
    }
    if (mViewsUnresolved != 0) {
        return;
    }

    if (mShownText == kNoText) {
        mShownText = text;
        StartShow(flTime);
    } else {
        StartHide(flTime);
        mWaitingText = text;
    }
}

// 0x003130e8
void MetHelpScreen::ClearInfoTexts() {
    for (std::vector<Rnd::Text *>::size_type i = 0; i < mInfoTexts.size(); ++i) {
        mInfoTexts[i]->SetText(HxStr(kNoText));
    }
    mView->UpdateWorldXfm(nullptr, 1); // Yes, the binary discards the result.
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
                std::memcpy(texts[i]->mLocalXfm[kTranslationRow], &end, sizeof(end));
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
        mView->UpdateWorldXfm(nullptr, 1); // Yes, the binary discards the result.
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
void MetHelpScreen::OnExitFinished() {
    ClearInfoTexts();
    mShownText = kNoText;
    mWaitingText = kNoText;
    mHideStart = 0.0f;
}

// 0x00317508
void MetHelpScreen::StartShow(float flTime) {
    if (mShowStart == 0.0f) {
        mShowStart = flTime;
        FillTexts(mShownText, mInfoTexts, mInfoOrigin, kFillInfoTexts);
        mShowAnim->SetFrame(0.0f);
    }
}

// 0x00317568
void MetHelpScreen::UpdateShow(float flTime) {
    if (mShowStart == 0.0f) {
        return;
    }
    if (mShowAnim != nullptr) {
        mShowAnim->SetFrame(flTime - mShowStart);
    }
    if (mShowStart + mShowEnd < flTime) {
        mShowStart = 0.0f;
        if (mWaitingText != kNoText) {
            StartHide(flTime);
        }
    }
}

// 0x00317600
void MetHelpScreen::StartHide(float flTime) {
    if (mHideStart == 0.0f) {
        mHideStart = flTime;
        mHideAnim->SetFrame(0.0f);
    }
}

// 0x00317640
void MetHelpScreen::UpdateHide(float flTime) {
    if (mHideStart == 0.0f) {
        return;
    }
    if (mHideAnim != nullptr) {
        mHideAnim->SetFrame(flTime - mHideStart);
    }
    if (mHideStart + mHideEnd < flTime) {
        mHideStart = 0.0f;
        mShownText = mWaitingText;
        mWaitingText = kNoText;
        if (mShownText != kNoText) {
            StartShow(flTime);
        } else {
            ClearInfoTexts();
        }
    }
}

// 0x00317758
void MetHelpScreen::ApplyPreset(const HxStr &name) {
    mPreset = name;
    if (mViewsUnresolved == 0) {
        FillTexts(name, mTitleTexts, mTitleOrigin, kFillTitleTexts);
    }
}
