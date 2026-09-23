#include "met/metkeyboardscreen.h"

#include <algorithm>
#include <climits>
#include <ctype.h>
#include <string.h>

#include "app/playsound.h"
#include "math/vector3.h"
#include "met/methelpscreen.h"
#include "met/metkbuser.h"
#include "met/metkeyboardrequest.h"
#include "met/metrenderer.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/button.h"
#include "rnd/manager.h"
#include "script/configquery.h"

namespace {

// 0x006a7c80
std::vector<HxStr> g_defaultMacros;

// 0x006a7c8c
// The widest the entered text may measure, which Open() copies from the request.
int g_nKeyboardMaxWidth = 500;

// 0x006a7c90
// The most characters the entry accepts, which Open() copies from the request.
int g_nKeyboardMaxLength = INT_MAX;

// The screen the keyboard registers under.
static const char *const kKeyboardScreen = "MetKeyboardScreen";
// The name of no screen and no key.
static const char *const kNoName = "";

// The number of default macros, one for each function key.
constexpr int kDefaultMacroCount = 12;
static const char *const kMacroKeyFormat = "kb_macro_f%i";
constexpr int kMacroConfigCode = 0x258;

// Posted to the ticker when the keyboard departs.
static const char *const kClearTickerTemplate = "keyboard_clear_ticker";
// Posted to the ticker when a macro does not fit.
static const char *const kNoMacroRoomTicker = "Not enough room to insert Macro.";
// The two key clicks. The first accompanies sliding and the second every other navigation.
static const char *const kKeySound1 = "SND_MET_KEY1";
static const char *const kKeySound2 = "SND_MET_KEY2";

// mSelector when the screen accepts every selector.
constexpr int kSelectorAny = -1;

// The pieces of a key button's name.
static const char *const kKeyButtonPrefix = "key_";
static const char *const kKeyButtonSuffix = ".but";
static const char *const kKeyButtonUpperSuffix = "_cap.but";
static const char *const kKeyButtonLowerSuffix = "_low.but";
// Longer names are never a single letter.
constexpr unsigned kSingleCharacterLength = 1;

// Rnd::Button states.
constexpr int kButtonStateNormal = 0;
constexpr int kButtonStateHighlighted = 1;
constexpr int kButtonStateLatched = 2;
constexpr int kButtonStateDisabled = 3;

// The layout grid.
constexpr int kKeyRowLength = 16;
constexpr int kKeyRowCount = 6;
// Rows of letters and symbols in each layout, between the function row and the space row.
constexpr int kLayoutRowCount = 4;

// mShiftState values.
enum KeyboardShiftState {
    kShiftStateRegular = 0,
    kShiftStateShift = 1,
    kShiftStateCaps = 2,
};

// mLastAction values, which slot 28 turns into a sound.
enum KeyboardAction {
    kActionCycleLeft = 0,
    kActionCycleRight = 1,
    kActionHigh = 2,
    kActionAccepted = 3,
    kActionRejected = 4,
    kActionShift = 5,
    kActionIdle = 6,
};

// The selection EnterAndShow() starts on.
constexpr int kStartRow = 3;
constexpr int kStartColumn = 13;

// Renderer time between caret toggles, and the value that starts the blink.
constexpr float kBlinkInterval = 240.0f;
constexpr float kBlinkStart = 1.0f;
constexpr float kBlinkOff = 0.0f;

// The alternation a key press runs.
constexpr float kPressInterval = 5.0f;
constexpr int kPressCycles = 1;

// The spaces the tab key inserts.
constexpr int kTabWidth = 3;
// The length the tab key tests against the limit, which is one short of kTabWidth.
constexpr unsigned kTabLengthMargin = 2;

// MetScreen::mUnknown18 when the text is committed.
constexpr int kExitCommit = 2;
// MetScreen::mUnknown18 when the keyboard departs without committing.
constexpr int kExitCancel = 0;

// The command codes slot 19 recognises beyond the six MetScreenCommandCode values.
constexpr int kCommandEnter = 7;
constexpr int kCommandSpace = 8;
constexpr int kCommandCaretLeft = 11;
constexpr int kCommandBackspace = 12;
constexpr int kCommandCaretRight = 13;
constexpr int kCommandShift = 20;
constexpr int kCommandCaps = 21;

// The padding word of a translation.
constexpr float kVectorPadding = 1.0f;

// Row of Rnd::Transformable::mLocalXfm that holds the translation.
constexpr int kTranslationRow = 3;

// 0x00891b20
HxStr g_textTooWide("met_keyboard_text_too_wide");
HxStr g_macroTooLarge("met_keyboard_macro_too_large.");

// 0x00891b30
// The names of the keys that are not typed as a character.
HxStr g_keyBackspace("BACKSPACE");
HxStr g_keyTab("TAB");
HxStr g_keyCaps("CAPS");
HxStr g_keyEnter("ENTER");
HxStr g_keyShift("SHIFT");
HxStr g_keyShift2("SHIFT2");
HxStr g_keySpace("SPACE");
HxStr g_keyCaretLeft("arr_l");
HxStr g_keyCaretRight("arr_r");
HxStr g_keyDelete("DELETE");
HxStr g_keyF1("F1");
HxStr g_keyF2("F2");
HxStr g_keyF3("F3");
HxStr g_keyF4("F4");
HxStr g_keyF5("F5");
HxStr g_keyF6("F6");
HxStr g_keyF7("F7");
HxStr g_keyF8("F8");
HxStr g_keyF9("F9");
HxStr g_keyF10("F10");
HxStr g_keyF11("F11");
HxStr g_keyF12("F12");

// 0x00891be0
// The function keys, in macro order.
HxStr g_macroKeys[] = {g_keyF1,
                       g_keyF2,
                       g_keyF3,
                       g_keyF4,
                       g_keyF5,
                       g_keyF6,
                       g_keyF7,
                       g_keyF8,
                       g_keyF9,
                       g_keyF10,
                       g_keyF11,
                       g_keyF12};

// 0x00891c40
// The first row of every layout.
HxStr g_functionRow[] = {g_keyF1,
                         g_keyF1,
                         g_keyF2,
                         g_keyF3,
                         g_keyF4,
                         g_keyF5,
                         g_keyF5,
                         g_keyF6,
                         g_keyF7,
                         g_keyF8,
                         g_keyF9,
                         g_keyF9,
                         g_keyF10,
                         g_keyF11,
                         g_keyF12,
                         g_keyF12};

// 0x00891cc0
// The last row of every layout.
HxStr g_spaceRow[] = {g_keySpace,
                      g_keySpace,
                      g_keySpace,
                      g_keySpace,
                      g_keySpace,
                      g_keySpace,
                      g_keySpace,
                      g_keySpace,
                      g_keySpace,
                      g_keySpace,
                      g_keySpace,
                      g_keyCaretLeft,
                      g_keyCaretRight,
                      g_keyDelete,
                      g_keyDelete,
                      g_keyDelete};

// 0x00891d40
HxStr g_regularKeys[kLayoutRowCount][kKeyRowLength] = {
    {"`",
     "1",
     "2",
     "3",
     "4",
     "5",
     "6",
     "7",
     "8",
     "9",
     "0",
     "-",
     "=",
     g_keyBackspace,
     g_keyBackspace,
     g_keyBackspace},
    {g_keyTab,
     g_keyTab,
     g_keyTab,
     "q",
     "w",
     "e",
     "r",
     "t",
     "y",
     "u",
     "i",
     "o",
     "p",
     "[",
     "]",
     "\\"},
    {g_keyCaps,
     g_keyCaps,
     "a",
     "s",
     "d",
     "f",
     "g",
     "h",
     "j",
     "k",
     "l",
     ";",
     "'",
     g_keyEnter,
     g_keyEnter,
     g_keyEnter},
    {g_keyShift,
     g_keyShift,
     g_keyShift,
     "z",
     "x",
     "c",
     "v",
     "b",
     "n",
     "m",
     ",",
     ".",
     "/",
     g_keyShift2,
     g_keyShift2,
     g_keyShift2},
};

// 0x00891f40
HxStr g_shiftKeys[kLayoutRowCount][kKeyRowLength] = {
    {"~",
     "!",
     "@",
     "#",
     "$",
     "%",
     "^",
     "&",
     "*",
     "(",
     ")",
     "_",
     "+",
     g_keyBackspace,
     g_keyBackspace,
     g_keyBackspace},
    {g_keyTab, g_keyTab, g_keyTab, "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "{", "}", "|"},
    {g_keyCaps,
     g_keyCaps,
     "A",
     "S",
     "D",
     "F",
     "G",
     "H",
     "J",
     "K",
     "L",
     ":",
     "\"",
     g_keyEnter,
     g_keyEnter,
     g_keyEnter},
    {g_keyShift,
     g_keyShift,
     g_keyShift,
     "Z",
     "X",
     "C",
     "V",
     "B",
     "N",
     "M",
     "<",
     ">",
     "?",
     g_keyShift2,
     g_keyShift2,
     g_keyShift2},
};

// 0x00892140
HxStr g_capsKeys[kLayoutRowCount][kKeyRowLength] = {
    {"`",
     "1",
     "2",
     "3",
     "4",
     "5",
     "6",
     "7",
     "8",
     "9",
     "0",
     "-",
     "=",
     g_keyBackspace,
     g_keyBackspace,
     g_keyBackspace},
    {g_keyTab,
     g_keyTab,
     g_keyTab,
     "Q",
     "W",
     "E",
     "R",
     "T",
     "Y",
     "U",
     "I",
     "O",
     "P",
     "[",
     "]",
     "\\"},
    {g_keyCaps,
     g_keyCaps,
     "A",
     "S",
     "D",
     "F",
     "G",
     "H",
     "J",
     "K",
     "L",
     ";",
     "'",
     g_keyEnter,
     g_keyEnter,
     g_keyEnter},
    {g_keyShift,
     g_keyShift,
     g_keyShift,
     "Z",
     "X",
     "C",
     "V",
     "B",
     "N",
     "M",
     ",",
     ".",
     "/",
     g_keyShift2,
     g_keyShift2,
     g_keyShift2},
};

// 0x006a7c98
HxStr *g_regularRows[kKeyRowCount] = {g_functionRow,
                                      g_regularKeys[0],
                                      g_regularKeys[1],
                                      g_regularKeys[2],
                                      g_regularKeys[3],
                                      g_spaceRow};

// 0x006a7cb0
HxStr *g_shiftRows[kKeyRowCount] = {
    g_functionRow, g_shiftKeys[0], g_shiftKeys[1], g_shiftKeys[2], g_shiftKeys[3], g_spaceRow};

// 0x006a7cc8
HxStr *g_capsRows[kKeyRowCount] = {
    g_functionRow, g_capsKeys[0], g_capsKeys[1], g_capsKeys[2], g_capsKeys[3], g_spaceRow};

// The text of an HxStr, with the shared empty string standing in for a null buffer.
inline const char *TextOf(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

// The screen name, the directory the container loads from, and the container name.
static const char *const kScreenName = "kb";
static const char *const kDirectory = "metagame/Shared";
static const char *const kContainerName = "keyboard";

// The text the constructor starts mText at.
static const char *const kNoText = "";

// The seven container objects slot 38 resolves.
static const char *const kCursorObject = "cursor.txt";
static const char *const kTextEntryObject = "text entry window.txt";
static const char *const kTitleBarObject = "title bar.txt";
static const char *const kMacroDisplayObject = "macro_display.txt";
static const char *const kRegularPanelObject = "keypanel_regular.view";
static const char *const kShiftPanelObject = "keypanel_shift.view";
static const char *const kCapsPanelObject = "keypanel_caps.view";

// The selection slot 38 leaves before EnterAndShow() moves it.
constexpr int kResolvedRow = 3;
constexpr int kResolvedColumn = 6;

// What the constructor leaves in mMacrosDisabled and in two MetScreen members.
constexpr int kMacrosDisabled = 1;
constexpr float kBaseUnknown58 = 1.0f;
constexpr int kBaseUnknown5c = 0;

inline Rnd::Text *FindText(const char *pszName) {
    Rnd::Object *pObject = Rnd::g_manager.Find(HxStr(pszName));
    return pObject != nullptr ? dynamic_cast<Rnd::Text *>(pObject) : nullptr;
}

inline Rnd::View *FindView(const char *pszName) {
    Rnd::Object *pObject = Rnd::g_manager.Find(HxStr(pszName));
    return pObject != nullptr ? dynamic_cast<Rnd::View *>(pObject) : nullptr;
}

} // namespace

// 0x00282660
MetKeyboardScreen::MetKeyboardScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mMacros(nullptr), mpKeypanelRegular(nullptr), mpKeypanelShift(nullptr),
      mpKeypanelCaps(nullptr), mpCursor(nullptr), mpTextEntryWindow(nullptr), mpTitleBar(nullptr),
      mpMacroDisplay(nullptr), mText(kNoText), mCaret(0), mBlinkTime(0.0f), mUser(nullptr),
      mRows(nullptr), mPanel(nullptr), mRow(0), mColumn(0), mShiftState(kShiftStateRegular),
      mMacrosDisabled(kMacrosDisabled), mLastAction(kActionShift) {
    mCursorOffset.w = kVectorPadding;
    mMacroOffset.w = kVectorPadding;
    GetDefaultMacros(); // Yes, the binary discards this call's result.
    mUnknown58 = kBaseUnknown58;
    mUnknown5c = kBaseUnknown5c;
}

// 0x00282e30
MetKeyboardScreen::~MetKeyboardScreen() {
}

// 0x00282948
void MetKeyboardScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    mpCursor = FindText(kCursorObject);
    // Yes, the binary does not test the cursor or the macro display for null.
    memcpy(&mCursorOffset, mpCursor->mLocalXfm[kTranslationRow], sizeof(mCursorOffset));
    mpTextEntryWindow = FindText(kTextEntryObject);
    mpTextEntryWindow->SetText(mText);
    mpTitleBar = FindText(kTitleBarObject);
    mpMacroDisplay = FindText(kMacroDisplayObject);
    memcpy(&mMacroOffset, mpMacroDisplay->mLocalXfm[kTranslationRow], sizeof(mMacroOffset));
    mpKeypanelRegular = FindView(kRegularPanelObject);
    mpKeypanelShift = FindView(kShiftPanelObject);
    mpKeypanelCaps = FindView(kCapsPanelObject);
    mPanel = mpKeypanelRegular;
    mRows = g_regularRows;
    mRow = kResolvedRow;
    mColumn = kResolvedColumn;
    ResetKeyStates();
}

// 0x00282ef0
void MetKeyboardScreen::DispatchKeyName(const HxStr &name) {
    if (name.mLen == 0) {
        return;
    }
    if (name == g_keyCaps) {
        OnCaps();
    } else if (name == g_keyTab) {
        OnTab();
    } else if (name == g_keySpace) {
        OnSpace();
    } else if (name == g_keyShift || name == g_keyShift2) {
        OnShift();
    } else if (name == g_keyEnter) {
        OnEnter();
    } else if (name == g_keyDelete) {
        OnDelete();
    } else if (name == g_keyBackspace) {
        OnBackspace();
    } else if (name == g_keyCaretLeft) {
        OnCaretLeft();
    } else if (name == g_keyCaretRight) {
        OnCaretRight();
    } else if (name == g_keyF1) {
        OnMacro(0);
    } else if (name == g_keyF2) {
        OnMacro(1);
    } else if (name == g_keyF3) {
        OnMacro(2);
    } else if (name == g_keyF4) {
        OnMacro(3);
    } else if (name == g_keyF5) {
        OnMacro(4);
    } else if (name == g_keyF6) {
        OnMacro(5);
    } else if (name == g_keyF7) {
        OnMacro(6);
    } else if (name == g_keyF8) {
        OnMacro(7);
    } else if (name == g_keyF9) {
        OnMacro(8);
    } else if (name == g_keyF10) {
        OnMacro(9);
    } else if (name == g_keyF11) {
        OnMacro(10);
    } else if (name == g_keyF12) {
        OnMacro(11);
    } else {
        OnCharacter(name);
    }
}

// 0x00283268
void MetKeyboardScreen::HandleCommand(const MetScreenCommand *pCommand) {
    if (mSelector != kSelectorAny && mSelector != pCommand->mPadIndex) {
        return;
    }

    SetPendingCommand(HxStr(kNoName));
    switch (pCommand->mCommand) {
    case kMetScreenCommandPrevious:
        PlaySlideSound(mSelector);
        MoveUp();
        break;

    case kMetScreenCommandNext:
        PlaySlideSound(mSelector);
        MoveDown();
        break;

    case kMetScreenCommandLeft:
        PlaySlideSound(mSelector);
        MoveLeft();
        break;

    case kMetScreenCommandRight:
        PlaySlideSound(mSelector);
        MoveRight();
        break;

    case kMetScreenCommandSelect:
        // The binary resolves the selected key twice rather than once.
        SetPendingCommand(*CurrentKey());
        StartRepeatingSound(
            mUnknown10->mUnknown68, kPressInterval, FindKeyButton(*CurrentKey()), kPressCycles);
        ActivateNamedPanel(HxStr(kNoName));
        break;

    case kMetScreenCommandBack:
        mBlinkTime = kBlinkOff;
        mUnknown18 = kExitCancel;
        BeginExit();
        break;

    case kCommandEnter:
        PressKey(g_keyEnter);
        break;

    case kCommandSpace:
        PressKey(g_keySpace);
        break;

    case kCommandCaretLeft:
        PressKey(g_keyCaretLeft);
        break;

    case kCommandBackspace:
        PressKey(g_keyBackspace);
        break;

    case kCommandCaretRight:
        PressKey(g_keyCaretRight);
        break;

    case kCommandShift:
        DispatchKeyName(g_keyShift);
        break;

    case kCommandCaps:
        DispatchKeyName(g_keyCaps);
        break;

    default:
        break;
    }
}

// 0x00283868
void MetKeyboardScreen::EnterAndShow() {
    mpTitleBar->SetText(mPrompt);
    mpTextEntryWindow->SetText(mText);
    mCaret = mText.mLen;
    UpdateCursor();
    mRow = kStartRow;
    mColumn = kStartColumn;
    // Toggling shift twice restores the regular layout and the highlight.
    OnShift();
    OnShift();
    mBlinkTime = kBlinkStart;
    MetScreen::EnterAndShow();
}

// 0x00283968
void MetKeyboardScreen::OnUnknownSlot30([[maybe_unused]] Rnd::Button *pButton) {
    if (mPendingKey.mLen == 0) {
        return;
    }
    UnhighlightKey(mPendingKey);
    UnhighlightKey(*CurrentKey());
    HighlightCurrentKey();
    SetPendingCommand(HxStr(kNoName));
    ActivateNamedPanel(HxStr(kKeyboardScreen));
}

// 0x00283aa0
void MetKeyboardScreen::OnUnknownSlot36() {
    mBlinkTime = kBlinkOff;
    UnhighlightKey(HxStr(*CurrentKey()));

    if (mUnknown18 != 0) {
        bool bTrimming = true;
        while (bTrimming) {
            if (static_cast<unsigned>(mText.ReverseFind(' ')) == g_nHxStrNoPosition) {
                bTrimming = false;
            } else {
                const int nSpace = mText.ReverseFind(' ');
                if (nSpace == static_cast<int>(mText.mLen - 1)) {
                    mText.Erase(nSpace, 1);
                } else {
                    bTrimming = false;
                }
            }
        }
        mUser->OnUnknownSlot2(mText);
    }

    FindScreenByName(mReturnScreen)->OnKeyboardDismissed();
    ActivateNamedPanel(mReturnScreen);
    OnDeparted();
    mShiftState = kShiftStateRegular;
}

// 0x00283c10
void MetKeyboardScreen::ResetKeyStates() {
    mMacrosDisabled = 1;
    HxStr path;
    for (int i = 0; i < kDefaultMacroCount; ++i) {
        HxStr key(g_macroKeys[i]);
        path = HxStr(kKeyButtonPrefix) + key + kKeyButtonSuffix;
        dynamic_cast<Rnd::Button *>(Rnd::g_manager.Find(path))->SetState(kButtonStateDisabled);
    }
}

// 0x00285378
inline Rnd::Button *MetKeyboardScreen::FindKeyButton(const HxStr &key) {
    HxStr path;
    if (key.mLen <= kSingleCharacterLength) {
        if (isupper(key[0])) {
            path = HxStr(kKeyButtonPrefix) + key + kKeyButtonUpperSuffix;
        } else if (islower(key[0])) {
            path = HxStr(kKeyButtonPrefix) + key + kKeyButtonLowerSuffix;
        } else {
            path = HxStr(kKeyButtonPrefix) + key + kKeyButtonSuffix;
        }
    } else {
        path = HxStr(kKeyButtonPrefix) + key + kKeyButtonSuffix;
    }
    return dynamic_cast<Rnd::Button *>(Rnd::g_manager.Find(path));
}

// 0x0028c870
inline HxStr *MetKeyboardScreen::CurrentKey() {
    return &mRows[mRow][mColumn];
}

// 0x0028caf8
inline void MetKeyboardScreen::HideMacro() {
    mpMacroDisplay->SetText(HxStr(kNoName));
    memcpy(mpMacroDisplay->mLocalXfm[kTranslationRow], &mMacroOffset, sizeof(mMacroOffset));
    mpMacroDisplay->mDirty = 1;
}

// 0x0028c898
inline void MetKeyboardScreen::UnhighlightKey(const HxStr &key) {
    if (key.mLen == 0) {
        return;
    }
    if (mShiftState == kShiftStateCaps && key == g_keyCaps) {
        FindKeyButton(g_keyCaps)->SetState(kButtonStateLatched);
    } else if (mShiftState == kShiftStateShift && key == g_keyShift) {
        FindKeyButton(g_keyShift)->SetState(kButtonStateLatched);
    } else if (mShiftState == kShiftStateShift && key == g_keyShift2) {
        FindKeyButton(g_keyShift2)->SetState(kButtonStateLatched);
    } else {
        FindKeyButton(key)->SetState(kButtonStateNormal);
    }
    HideMacro();
}

// 0x0028cc50
bool MetKeyboardScreen::IsMacroKey(const HxStr &key) {
    for (int i = 0; i < kDefaultMacroCount; ++i) {
        if (g_macroKeys[i] == key) {
            return true;
        }
    }
    return false;
}

// 0x0028ccb8
void MetKeyboardScreen::SetPendingCommand(const HxStr &key) {
    mPendingKey = key;
}

// 0x0028cd00
inline void MetKeyboardScreen::SetTickerText(const HxStr &text) {
    static HxStr sTicker(kNoName);
    if (!(sTicker == text)) {
        sTicker = text;
        MetHelpScreen::SetText(sTicker, mUnknown10->mUnknown68);
    }
}

// 0x0028c3e8
inline void MetKeyboardScreen::UpdateCursor() {
    mpTextEntryWindow->SetShowing(1);
    Vector3 position = mpTextEntryWindow->CharPosition(mCaret);
    Vector3 cursor;
    cursor.w = kVectorPadding;
    AddVec3(&mCursorOffset.x, &position.x, &cursor.x);
    memcpy(mpCursor->mLocalXfm[kTranslationRow], &cursor, sizeof(cursor));
    mpCursor->mDirty = 1;
}

// 0x0028ca58
inline int MetKeyboardScreen::TextEndX() {
    mpTextEntryWindow->SetShowing(1);
    return static_cast<int>(mpTextEntryWindow->CharPosition(mText.mLen).x);
}

// 0x0028c9c8
inline void MetKeyboardScreen::RemoveChar(int nIndex) {
    mText.Erase(nIndex, 1);
}

// 0x0028c9e8
inline void MetKeyboardScreen::AppendText(const HxStr &text) {
    mText += text;
}

// 0x0028ca08
inline void MetKeyboardScreen::InsertText(const HxStr &text, unsigned nPos) {
    if (nPos < mText.mLen) {
        mText.Insert(nPos, text);
    } else {
        mText += text;
    }
}

inline void MetKeyboardScreen::PressKey(const HxStr &key) {
    SetPendingCommand(key);
    StartRepeatingSound(mUnknown10->mUnknown68, kPressInterval, FindKeyButton(key), kPressCycles);
    ActivateNamedPanel(HxStr(kNoName));
}

// 0x00283f38
void MetKeyboardScreen::HighlightCurrentKey() {
    HxStr key(*CurrentKey());
    if (IsMacroKey(key) && mMacrosDisabled != 0) {
        return;
    }
    FindKeyButton(key)->SetState(kButtonStateHighlighted);
    ShowMacro(key);
}

// 0x00284788
void MetKeyboardScreen::MoveRight() {
    HxStr previous(*CurrentKey());
    HxStr current(previous);
    UnhighlightKey(previous);
    while (current == previous) {
        if (++mColumn >= kKeyRowLength) {
            mColumn -= kKeyRowLength;
        }
        current = *CurrentKey();
    }
    HighlightCurrentKey();
}

// 0x002848e0
void MetKeyboardScreen::MoveLeft() {
    HxStr previous(*CurrentKey());
    HxStr current(previous);
    UnhighlightKey(previous);
    while (current == previous) {
        if (--mColumn < 0) {
            mColumn += kKeyRowLength;
        }
        current = *CurrentKey();
    }
    HighlightCurrentKey();
}

// 0x00284a30
void MetKeyboardScreen::MoveDown() {
    HxStr previous(*CurrentKey());
    HxStr current(previous);
    UnhighlightKey(previous);
    while (current == previous || (IsMacroKey(current) && mMacrosDisabled == 1)) {
        if (++mRow >= kKeyRowCount) {
            mRow -= kKeyRowCount;
        }
        current = *CurrentKey();
    }
    HighlightCurrentKey();
}

// 0x00284bb0
void MetKeyboardScreen::MoveUp() {
    HxStr previous(*CurrentKey());
    HxStr current(previous);
    UnhighlightKey(previous);
    while (current == previous || (IsMacroKey(current) && mMacrosDisabled == 1)) {
        if (--mRow < 0) {
            mRow += kKeyRowCount;
        }
        current = *CurrentKey();
    }
    HighlightCurrentKey();
}

// 0x00284d30
std::vector<HxStr> *MetKeyboardScreen::GetDefaultMacros() {
    if (g_defaultMacros.size() != kDefaultMacroCount) {
        g_defaultMacros.resize(kDefaultMacroCount);
        for (int i = 0; i < kDefaultMacroCount; ++i) {
            g_defaultMacros[i] = DefaultMacro(i);
        }
    }
    return &g_defaultMacros;
}

// 0x00284ec0
void MetKeyboardScreen::ShowMacro(const HxStr &key) {
    if (mMacrosDisabled != 0) {
        return;
    }
    for (int i = 0; i < kDefaultMacroCount; ++i) {
        if (!(g_macroKeys[i] == key)) {
            continue;
        }

        const HxStr &macro = (*mMacros)[i];
        const int nEnd = TextEndX();
        const float flWidth = mpTextEntryWindow->MeasureText(TextOf(macro), macro.mLen);
        // Yes, the binary reports an overflow only while the length still fits.
        if (static_cast<float>(g_nKeyboardMaxWidth) <= static_cast<float>(nEnd) + flWidth &&
            mText.mLen + macro.mLen < static_cast<unsigned>(g_nKeyboardMaxLength)) {
            PlayErrorSound(mSelector);
            SetTickerText(g_macroTooLarge);
            return;
        }

        SetTickerText(mTicker);
        mpTextEntryWindow->SetShowing(1);
        Vector3 position = mpTextEntryWindow->CharPosition(mText.mLen);
        Vector3 caption;
        caption.w = kVectorPadding;
        AddVec3(&mMacroOffset.x, &position.x, &caption.x);
        memcpy(mpMacroDisplay->mLocalXfm[kTranslationRow], &caption, sizeof(caption));
        mpMacroDisplay->mDirty = 1;
        mpMacroDisplay->SetText(macro);
        return;
    }
}

// 0x002850b8
void MetKeyboardScreen::InsertMacro(int nIndex) {
    if (mMacrosDisabled != 0) {
        return;
    }

    const HxStr &macro = (*mMacros)[nIndex];
    const int nEnd = TextEndX();
    const float flWidth = mpTextEntryWindow->MeasureText(TextOf(macro), macro.mLen);
    // Yes, the binary rejects the macro only while the length still fits.
    if (static_cast<float>(g_nKeyboardMaxWidth) <= static_cast<float>(nEnd) + flWidth &&
        mText.mLen + macro.mLen < static_cast<unsigned>(g_nKeyboardMaxLength)) {
        mLastAction = kActionRejected;
        SetTickerText(HxStr(kNoMacroRoomTicker));
        return;
    }

    mLastAction = kActionAccepted;
    SetTickerText(mTicker);
    // Yes, the binary appends the default macro rather than the offered one.
    mText += DefaultMacro(nIndex);
    mCaret = mText.mLen;
    HideMacro();
}

// 0x00285b08
void MetKeyboardScreen::OnShift() {
    UnhighlightKey(*CurrentKey());
    mLastAction = kActionShift;
    PlaySlideSound(mSelector);
    SetTickerText(mTicker);

    switch (mShiftState) {
    case kShiftStateRegular:
    case kShiftStateCaps:
        mShiftState = kShiftStateShift;
        mPanel->SetShowing(0);
        mPanel = mpKeypanelShift;
        mPanel->SetShowing(1);
        mRows = g_shiftRows;
        FindKeyButton(g_keyCaps)->SetState(kButtonStateNormal);
        FindKeyButton(g_keyShift)->SetState(kButtonStateLatched);
        FindKeyButton(g_keyShift2)->SetState(kButtonStateLatched);
        break;

    case kShiftStateShift:
        mShiftState = kShiftStateRegular;
        mPanel->SetShowing(0);
        mPanel = mpKeypanelRegular;
        mPanel->SetShowing(1);
        mRows = g_regularRows;
        FindKeyButton(g_keyShift)->SetState(kButtonStateNormal);
        FindKeyButton(g_keyShift2)->SetState(kButtonStateNormal);
        break;

    default:
        break;
    }
    HighlightCurrentKey();
}

// 0x00285e28
void MetKeyboardScreen::OnBackspace() {
    if (mCaret > 0) {
        RemoveChar(mCaret - 1);
        mLastAction = kActionAccepted;
        --mCaret;
        SetTickerText(mTicker);
    } else {
        mLastAction = kActionRejected;
    }
    mpTextEntryWindow->SetText(mText);
    UpdateCursor();
}

// 0x00285fa0
void MetKeyboardScreen::OnCaretLeft() {
    mLastAction = mCaret > 0 ? kActionAccepted : kActionRejected;
    SetTickerText(mTicker);
    mCaret = std::max(0, mCaret - 1);
    (void)TextEndX(); // Yes, the binary discards this measurement.
    mpTextEntryWindow->SetText(mText);
    UpdateCursor();
}

// 0x00286120
void MetKeyboardScreen::OnCaretRight() {
    mLastAction = static_cast<unsigned>(mCaret) < mText.mLen ? kActionAccepted : kActionRejected;
    SetTickerText(mTicker);
    mCaret = std::min(mCaret + 1, static_cast<int>(mText.mLen));
    (void)TextEndX(); // Yes, the binary discards this measurement.
    mpTextEntryWindow->SetText(mText);
    UpdateCursor();
}

// 0x002862b0
void MetKeyboardScreen::OnCaps() {
    Rnd::Button *pCaps = FindKeyButton(g_keyCaps);
    mLastAction = kActionShift;
    PlaySlideSound(mSelector);
    SetTickerText(mTicker);
    UnhighlightKey(*CurrentKey());

    switch (mShiftState) {
    case kShiftStateRegular:
    case kShiftStateShift:
        mShiftState = kShiftStateCaps;
        mPanel->SetShowing(0);
        mPanel = mpKeypanelCaps;
        mPanel->SetShowing(1);
        mRows = g_capsRows;
        pCaps->SetState(kButtonStateLatched);
        FindKeyButton(g_keyShift)->SetState(kButtonStateNormal);
        FindKeyButton(g_keyShift2)->SetState(kButtonStateNormal);
        break;

    case kShiftStateCaps:
        mShiftState = kShiftStateRegular;
        mPanel->SetShowing(0);
        mPanel = mpKeypanelRegular;
        mPanel->SetShowing(1);
        mRows = g_regularRows;
        pCaps->SetState(kButtonStateNormal);
        break;

    default:
        break;
    }
    HighlightCurrentKey();
}

// 0x002865a0
void MetKeyboardScreen::OnTab() {
    HxStr spaces;
    for (int i = 0; i < kTabWidth; ++i) {
        spaces += ' ';
    }

    const int nEnd = TextEndX();
    if (static_cast<float>(nEnd) + mpTextEntryWindow->MeasureText(TextOf(spaces), kTabWidth) <
            static_cast<float>(g_nKeyboardMaxWidth) &&
        mText.mLen + kTabLengthMargin < static_cast<unsigned>(g_nKeyboardMaxLength)) {
        InsertText(spaces, mCaret);
        mLastAction = kActionAccepted;
        mCaret += kTabWidth;
        SetTickerText(mTicker);
    } else {
        mLastAction = kActionRejected;
        SetTickerText(g_textTooWide);
    }
    mpTextEntryWindow->SetText(mText);
    UpdateCursor();
}

// 0x00286850
void MetKeyboardScreen::OnSpace() {
    const int nEnd = TextEndX();
    if (static_cast<float>(nEnd) + mpTextEntryWindow->MeasureText(" ", 1) <
            static_cast<float>(g_nKeyboardMaxWidth) &&
        mText.mLen < static_cast<unsigned>(g_nKeyboardMaxLength)) {
        mText.Insert(mCaret, 1, ' ');
        mLastAction = kActionAccepted;
        ++mCaret;
        SetTickerText(mTicker);
    } else {
        mLastAction = kActionRejected;
        SetTickerText(g_textTooWide);
    }
    mpTextEntryWindow->SetText(mText);
    UpdateCursor();
}

// 0x00286ab0
void MetKeyboardScreen::OnDelete() {
    if (static_cast<unsigned>(mCaret) < mText.mLen) {
        RemoveChar(mCaret);
        mLastAction = kActionAccepted;
        SetTickerText(mTicker);
    } else {
        mLastAction = kActionRejected;
    }
    mpTextEntryWindow->SetText(mText);
    UpdateCursor();
}

// 0x00286c10
void MetKeyboardScreen::OnCharacter(const HxStr &key) {
    char ch = key[0];
    const int nEnd = TextEndX();
    if (static_cast<float>(nEnd) + mpTextEntryWindow->MeasureText(&ch, 1) <
            static_cast<float>(g_nKeyboardMaxWidth) &&
        mText.mLen < static_cast<unsigned>(g_nKeyboardMaxLength)) {
        mText.Insert(mCaret, 1, ch);
        mLastAction = kActionAccepted;
        ++mCaret;
        SetTickerText(mTicker);
    } else {
        mLastAction = kActionRejected;
        SetTickerText(g_textTooWide);
    }
    mpTextEntryWindow->SetText(mText);
    UpdateCursor();
}

// 0x0028c2c0
void MetKeyboardScreen::SetKeyboardReturnScreen(const HxStr &returnScreen) {
    // The binary narrows the lookup without a runtime check.
    MetKeyboardScreen *pKeyboard =
        static_cast<MetKeyboardScreen *>(FindScreenByName(HxStr(kKeyboardScreen)));
    pKeyboard->SetReturnScreen(returnScreen);
}

// 0x0028c358
MetScreen *MetKeyboardScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetKeyboardScreen(pRenderer, nPriority);
}

// 0x0028c550
void MetKeyboardScreen::BeginExit() {
    SetTickerText(HxStr(kClearTickerTemplate));
    MetScreen::BeginExit();
}

// 0x0028c518
void MetKeyboardScreen::PlaySlideSound(int nSelector) {
    if (mSelector == nSelector || mSelector == kSelectorAny) {
        PlaySoundByName(kKeySound1);
    }
}

// 0x0028c4e0
void MetKeyboardScreen::PlayHighSound(int nSelector) {
    if (mSelector == nSelector || mSelector == kSelectorAny) {
        PlaySoundByName(kKeySound2);
    }
}

// 0x0028c470
void MetKeyboardScreen::PlayCycleLeftSound(int nSelector) {
    if (mSelector == nSelector || mSelector == kSelectorAny) {
        PlaySoundByName(kKeySound2);
    }
}

// 0x0028c4a8
void MetKeyboardScreen::PlayCycleRightSound(int nSelector) {
    if (mSelector == nSelector || mSelector == kSelectorAny) {
        PlaySoundByName(kKeySound2);
    }
}

// 0x0028c5e0
void MetKeyboardScreen::StartRepeatingSound(float flStartTime,
                                            float flInterval,
                                            Rnd::Button *pButton,
                                            int nCycles) {
    ResetCaret();
    if (mPendingKey.mLen == 0) {
        return;
    }
    DispatchKeyName(mPendingKey);
    switch (mLastAction) {
    case kActionCycleLeft:
        PlayCycleLeftSound(mSelector);
        break;

    case kActionCycleRight:
        PlayCycleRightSound(mSelector);
        break;

    case kActionHigh:
        PlayHighSound(mSelector);
        break;

    case kActionAccepted:
        PlaySlideSound(mSelector);
        break;

    case kActionRejected:
        PlayErrorSound(mSelector);
        break;

    default:
        break;
    }
    mLastAction = kActionIdle;
    MetScreen::StartRepeatingSound(flStartTime, flInterval, pButton, nCycles);
}

// 0x0028c708
void MetKeyboardScreen::OnUnknownSlot26(float flTime) {
    if (mBlinkTime == kBlinkOff || !(mBlinkTime + kBlinkInterval < flTime)) {
        return;
    }
    if (mpCursor->GetShowing() != 0) {
        mpCursor->SetShowing(0);
    } else {
        mpCursor->SetShowing(1);
    }
    mBlinkTime = flTime + kBlinkInterval;
}

// 0x0028c7c0
void MetKeyboardScreen::ResetCaret() {
    mpCursor->SetShowing(0);
    mBlinkTime = kBlinkStart;
}

// 0x0028c808
void MetKeyboardScreen::OnUnknownSlot33() {
    SetTickerText(mTicker);
}

// 0x0028c828
void MetKeyboardScreen::OnDeparted() {
}

// 0x0028cbb8
HxStr MetKeyboardScreen::DefaultMacro(int nIndex) {
    HxStr text = QueryConfigString(kMacroConfigCode, FormatString(kMacroKeyFormat, nIndex + 1));
    return text;
}

// 0x0028cda8
void MetKeyboardScreen::OnEnter() {
    mLastAction = kActionAccepted;
    mUnknown18 = kExitCommit;
    BeginExit();
    mpTextEntryWindow->SetText(mText);
    UpdateCursor();
}

// 0x0028ce70
void MetKeyboardScreen::OnMacro(int nIndex) {
    InsertMacro(nIndex);
    mpTextEntryWindow->SetText(mText);
    UpdateCursor();
}

// 0x00282468
void MetKeyboardScreen::Open(const MetKeyboardRequest &request) {
    // The binary narrows the lookup without a runtime check.
    MetKeyboardScreen *pKeyboard =
        static_cast<MetKeyboardScreen *>(MetScreen::FindScreenByName(HxStr(kKeyboardScreen)));
    pKeyboard->SetText(request.mText);
    pKeyboard->SetPrompt(request.mPrompt);
    pKeyboard->SetUser(request.mUser);
    pKeyboard->SetSelector(request.mPad);
    g_nKeyboardMaxWidth = request.mMaxWidth;
    g_nKeyboardMaxLength = request.mMaxLength;
    pKeyboard->mMacros = request.mMacros != nullptr ? request.mMacros : &g_defaultMacros;
    pKeyboard->SetTicker(request.mTicker);
    pKeyboard->SetReturnScreen(request.mReturnScreen);
    MetScreen *pReturn = MetScreen::FindScreenByName(request.mReturnScreen);
    pReturn->PushNamedScreen(HxStr(kKeyboardScreen));
    pReturn->ActivateNamedPanel(HxStr(kKeyboardScreen));
}

// 0x0028c830
void MetKeyboardScreen::SetText(const HxStr &text) {
    mText = text;
}

// 0x0028c850
void MetKeyboardScreen::SetPrompt(const HxStr &prompt) {
    mPrompt = prompt;
}

// 0x0028cad0
void MetKeyboardScreen::SetUser(MetKBUser *pUser) {
    mUser = pUser;
}

// 0x0028c3e0
void MetKeyboardScreen::SetSelector(int nSelector) {
    mSelector = nSelector;
}

// 0x0028cad8
void MetKeyboardScreen::SetTicker(const HxStr &ticker) {
    mTicker = ticker;
}

// 0x0028cab0
void MetKeyboardScreen::SetReturnScreen(const HxStr &returnScreen) {
    mReturnScreen = returnScreen;
}
