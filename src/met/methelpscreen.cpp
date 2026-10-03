#include "met/methelpscreen.h"

#include <cstring>
#include <utility>

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

#ifdef VIDEO_STANDARD_PAL
// A run of help text opens with a font code in angle brackets, such as `<F1>`.
static const char *const kFontCodeOpen = "<F";
static const char *const kFontCodeClose = ">";
static const char *const kPlainFontCode = "F1";
static const char *const kBlueFontCode = "F2";
static const char *const kControllerFontCode = "FC";
static const char *const kPlainFont = "font1_plain_3";
static const char *const kBlueFont = "font1_blue_1";
static const char *const kControllerFont = "big_controller.font";

// HxStr::Find() reports an absent text as this index.
constexpr int kNotFound = -1;
#endif

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

inline void EmptyTexts(std::vector<Rnd::Text *> &texts) {
    for (std::vector<Rnd::Text *>::size_type i = 0; i < texts.size(); ++i) {
        texts[i]->SetText(HxStr(kNoText));
    }
}

// Place one run of text at end in its font, and advance end past it.
inline void
PlaceText(Rnd::Text *pText, const HxStr &fontName, const HxStr &textValue, Vector3 &end) {
    std::memcpy(pText->mLocalXfm[kTranslationRow], &end, sizeof(end));
    pText->mDirty = 1;
    pText->SetFont(dynamic_cast<Rnd::Font *>(Rnd::g_manager.Find(fontName)));
    pText->SetText(textValue);
    Vector3 advance = pText->CharPosition(textValue.mLen);
    AddVec3(&end.x, &advance.x, &end.x);
}

// Shift the first nCount texts left by half the width of the run from origin to end.
inline void CentreTexts(std::vector<Rnd::Text *> &texts,
                        int nCount,
                        const Vector3 &origin,
                        const Vector3 &end) {
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

} // namespace

// NTSC-U/C: 0x003125b0, PAL: 0x00338418
MetHelpScreen::MetHelpScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mShowStart(0.0f), mHideStart(0.0f) {
    mInfoOrigin.w = kVectorPadding;
    mTitleOrigin.w = kVectorPadding;
    mShowsLoadedDrawables = 0;
}

// NTSC-U/C: 0x00312770, PAL: 0x00338640
MetHelpScreen::~MetHelpScreen() {
}

// NTSC-U/C: 0x003128d0, PAL: 0x003387d0
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

// NTSC-U/C: 0x00312df0, PAL: 0x00338da0
void MetHelpScreen::UpdateIdle(float flTime) {
    UpdateShow(flTime);
    UpdateHide(flTime);
}

// NTSC-U/C: 0x00312f70, PAL: 0x00338f20
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

// NTSC-U/C: 0x003130e8, PAL: 0x00339098
void MetHelpScreen::ClearInfoTexts() {
    for (std::vector<Rnd::Text *>::size_type i = 0; i < mInfoTexts.size(); ++i) {
        mInfoTexts[i]->SetText(HxStr(kNoText));
    }
    mView->UpdateWorldXfm(nullptr, 1); // Yes, the binary discards the result.
}

#ifdef VIDEO_STANDARD_PAL
// PAL: 0x003391d8
HxStr MetHelpScreen::FontForCode(const HxStr &code) {
    HxStr fontName(kPlainFont);
    if (code == kPlainFontCode) {
        fontName = kPlainFont;
    } else if (code == kBlueFontCode) {
        fontName = kBlueFont;
    } else if (code == kControllerFontCode) {
        fontName = kControllerFont;
    }
    return fontName;
}
#endif

// NTSC-U/C: 0x00313208, PAL: 0x00339300
void MetHelpScreen::FillTexts(const HxStr &key,
                              std::vector<Rnd::Text *> &texts,
                              const Vector3 &origin,
                              int nTitles) {
#ifdef VIDEO_STANDARD_PAL
    std::vector<std::pair<HxStr, HxStr>> runs;
    HxStr fontCode(kPlainFontCode);
    HxStr rest(key);
    while (rest.mLen != 0) {
        const int nOpen = rest.Find(kFontCodeOpen);
        if (nOpen == kNotFound) {
            break;
        }
        const int nClose = rest.Find(kFontCodeClose);
        if (nClose == kNotFound) {
            continue; // Yes, the binary loops forever on a font code with no closing bracket.
        }
        if (nOpen > 0) {
            std::pair<HxStr, HxStr> run;
            run.first = FontForCode(fontCode);
            run.second = rest.Mid(0, nOpen);
            runs.push_back(run);
        }
        fontCode = rest.Mid(nOpen + 1, nClose - 1 - nOpen);
        rest = rest.Mid(nClose + 1);
    }
    if (rest.mLen != 0) {
        std::pair<HxStr, HxStr> run;
        run.first = FontForCode(fontCode);
        run.second = rest;
        runs.push_back(run);
    }

    const int nCount = runs.size();
    if (nCount != 0) {
        Vector3 end = origin;
        EmptyTexts(texts);
        for (int i = 0; i < nCount; ++i) {
            const HxStr fontName(runs[i].first);
            const HxStr textValue(runs[i].second);
            PlaceText(texts[i], fontName, textValue, end);
        }
        CentreTexts(texts, nCount, origin, end);
    }
#else
    Py::Object result =
        EvalScriptTemplate(kHelpTextTemplate, key.mStr != nullptr ? key.mStr : g_szEmptyString);
    if (result.isList()) {
        Py::List entries(result);
        const int nCount = entries.length();
        Vector3 end = origin;
        EmptyTexts(texts);
        for (int i = 0; i < nCount; ++i) {
            if (entries[i].isTuple()) {
                Py::Tuple entry(entries[i]);
                Py::String font(entry[kEntryFieldFont]);
                Py::String value(entry[kEntryFieldText]);
                HxStr fontName = font.as_string();
                HxStr textValue = value.as_string();
                PlaceText(texts[i], fontName, textValue, end);
            }
        }
        CentreTexts(texts, nCount, origin, end);
    }
#endif

    if (nTitles == kFillInfoTexts) {
        mView->UpdateWorldXfm(nullptr, 1); // Yes, the binary discards the result.
    }
}

// NTSC-U/C: 0x00317210, PAL: 0x0033d4c8
MetHelpScreen *MetHelpScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetHelpScreen(pRenderer, nPriority);
}

// NTSC-U/C: 0x00317298, PAL: 0x00338328
void MetHelpScreen::SelectPreset(const HxStr &name) {
    FindHelpScreen()->ApplyPreset(name);
}

// NTSC-U/C: 0x00317368, PAL: 0x0033d550
void MetHelpScreen::SetText(const HxStr &text, float flTime) {
    FindHelpScreen()->PostText(text, flTime);
}

// NTSC-U/C: 0x00317448, PAL: 0x0033d650
void MetHelpScreen::HandleCommand(const MetScreenCommand *pCommand) {
    if (pCommand->mCommand == kMetScreenCommandBack) {
        BeginExit();
    }
}

// NTSC-U/C: 0x00317480, PAL: 0x0033d688
void MetHelpScreen::OnExitFinished() {
    ClearInfoTexts();
    mShownText = kNoText;
    mWaitingText = kNoText;
    mHideStart = 0.0f;
}

// NTSC-U/C: 0x00317508, PAL: 0x0033d710
void MetHelpScreen::StartShow(float flTime) {
    if (mShowStart == 0.0f) {
        mShowStart = flTime;
        FillTexts(mShownText, mInfoTexts, mInfoOrigin, kFillInfoTexts);
        mShowAnim->SetFrame(0.0f);
    }
}

// NTSC-U/C: 0x00317568, PAL: 0x0033d770
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

// NTSC-U/C: 0x00317600, PAL: 0x0033d808
void MetHelpScreen::StartHide(float flTime) {
    if (mHideStart == 0.0f) {
        mHideStart = flTime;
        mHideAnim->SetFrame(0.0f);
    }
}

// NTSC-U/C: 0x00317640, PAL: 0x0033d848
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

// NTSC-U/C: 0x00317758, PAL: 0x0033d960
void MetHelpScreen::ApplyPreset(const HxStr &name) {
    mPreset = name;
    if (mViewsUnresolved == 0) {
        FillTexts(name, mTitleTexts, mTitleOrigin, kFillTitleTexts);
    }
}
