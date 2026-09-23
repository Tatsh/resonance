#include "met/metfreqmakerinventoryscreen.h"

#include <algorithm>

#include "app/playsound.h"
#include "game/freqpart.h"
#include "game/freqparttemplate.h"
#include "math/color.h"
#include "math/vector2.h"
#include "met/metfreqmakerassetmanager.h"
#include "met/metfreqmakercanvasscreen.h"
#include "rnd/animatable.h"
#include "rnd/drawable.h"
#include "rnd/mesh.h"
#include "rnd/object.h"
#include "rnd/text.h"
#include "rnd/transformable.h"
#include "rnd/view.h"

namespace {

static const char *const kScreenName = "fm_inventory";
static const char *const kDirectory = "metagame/persona";
static const char *const kContainerName = "freq_maker_inventory";

static const char *const kMainInventoryViewName = "MainInventoryView";
static const char *const kBodyViewName = "BodyView";
static const char *const kHeadViewName = "HeadView";
static const char *const kFaceViewName = "FaceView";
static const char *const kDetailsViewName = "DetailsView";
static const char *const kLogosViewName = "LogosView";
static const char *const kEditViewName = "EditView";

static const char *const kDirectionsScreenName = "MetFreqMakerDirectionsScreen";
static const char *const kPanelName = "MetFreqMakerInventoryScreen";

static const char *const kToggleSound = "SND_MET_FM_TOGGLE";
static const char *const kDeleteSound = "SND_MET_FM_DELETE";
static const char *const kFlipSound = "SND_MET_FM_FLIP_BUBBLE";
static const char *const kPartSelectSound = "SND_MET_FM_PART_SELECT";
static const char *const kColorMoveSound = "SND_MET_FM_COLOR_MOVE";
static const char *const kPartMoveSound = "SND_MET_FM_PART_MOVE";

// The template categories, as FreqPartTemplate::mCategory records them. The names are inferred
// from the pages slot 38 lists each category on.
enum PartCategory {
    kPartCategoryBody = 1,
    kPartCategoryHead = 2,
    kPartCategoryDetails = 3,
    kPartCategoryFaceFirst = 4,
    kPartCategoryFaceSecond = 5,
    kPartCategoryFaceThird = 6,
    kPartCategoryLogos = 7,
    kPartCategoryDetailsSecond = 8,
    kPartCategoryDetailsThird = 9,
    kPartCategoryDetailsFourth = 10,
    kPartCategoryDetailsFifth = 11
};

// PartScale() results.
constexpr float kSmallPartScale = 1.0f;
constexpr float kLargePartScale = 2.0f;
constexpr float kUnknownPartScale = -1.0f;

// The part grid and the colour palette. The palette is sixteen cells wide and eight high, and a
// palette position runs from 0 to 1 across it.
constexpr int kGridColumnCount = 8;
constexpr int kMinimumRowCount = 3;
constexpr int kPaletteColumnCount = 16;
constexpr int kPaletteRowCount = 8;
constexpr float kPaletteCellWidth = 0.0625f;
constexpr float kPaletteCellHeight = 0.125f;
constexpr int kPaletteStartColumn = 8;
constexpr int kPaletteStartRow = 4;
constexpr float kPaletteCentre = 0.5f;
constexpr float kUnsetPalettePosition = -1.0f;

// The mapping from a palette position to the translation of `fm_cross_origin.view`.
constexpr int kXfmTranslationRow = 3;
constexpr float kCrossOriginWidth = 375.0f;
constexpr float kCrossOriginHeight = 140.0f;
constexpr float kCrossOriginDepth = -0.1f;
constexpr float kXfmTranslationW = 1.0f;

// MetScreen::mUnknown58 in each state of the screen.
constexpr float kConstructedUnknown58 = 3.0f;
constexpr float kIdleUnknown58 = 1.0f;

// One cell of the part grid.
struct GridCell {
    GridCell(int nColumn, int nRow) : mColumn(nColumn), mRow(nRow) {
    }

    int mColumn;
    int mRow;
};

// 0x00891af8 through 0x00891b17. The corners of the three visible rows of the grid. No routine
// reads them. The names are inferred.
GridCell g_gridTopLeft(0, 0);
GridCell g_gridBottomLeft(0, 2);
GridCell g_gridTopRight(7, 0);
GridCell g_gridBottomRight(7, 2);

// Hang pChild from pParent in front of every drawable already hung there.
inline void PrependView(Rnd::View *pParent, Rnd::View *pChild) {
    std::list<Rnd::Drawable *> &draws = pParent->GetDraws();
    pParent->AddDraw(pChild, draws.empty() ? nullptr : draws.front());
}

} // namespace

// 0x0026a498
MetFreqMakerInventoryScreen::MetFreqMakerInventoryScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mMainInventoryView(nullptr), mBodyView(nullptr), mHeadView(nullptr), mFaceView(nullptr),
      mDetailsView(nullptr), mLogosView(nullptr), mEditView(nullptr), mCurrentView(nullptr),
      mWire16(nullptr), mWire30(nullptr), mLimitText(nullptr), mPaletteColumn(0), mPaletteRow(0),
      mViewsResolved(0), mCanvas(nullptr), mCrossOrigin(nullptr), mGridColumn(0), mGridRow(0),
      mMode(kModeNone) {
    mUnknown58 = kConstructedUnknown58;
    mMainInventoryView = new Rnd::View(HxStr(kMainInventoryViewName));
    mBodyView = new Rnd::View(HxStr(kBodyViewName));
    mHeadView = new Rnd::View(HxStr(kHeadViewName));
    mFaceView = new Rnd::View(HxStr(kFaceViewName));
    mDetailsView = new Rnd::View(HxStr(kDetailsViewName));
    mLogosView = new Rnd::View(HxStr(kLogosViewName));
    mEditView = new Rnd::View(HxStr(kEditViewName));

    PrependView(mMainInventoryView, mBodyView);
    PrependView(mMainInventoryView, mHeadView);
    PrependView(mMainInventoryView, mFaceView);
    PrependView(mMainInventoryView, mDetailsView);
    PrependView(mMainInventoryView, mLogosView);
    PrependView(mMainInventoryView, mEditView);

    mHeadView->SetShowing(0);
    mFaceView->SetShowing(0);
    mBodyView->SetShowing(0);
    mDetailsView->SetShowing(0);
    mLogosView->SetShowing(0);
    mEditView->SetShowing(0);

    mMainInventoryView->AddTrans(mBodyView);
    mMainInventoryView->AddTrans(mHeadView);
    mMainInventoryView->AddTrans(mFaceView);
    mMainInventoryView->AddTrans(mDetailsView);
    mMainInventoryView->AddTrans(mLogosView);
    mMainInventoryView->AddTrans(mEditView);

    mMainInventoryView->AddAnim(mBodyView);
    mMainInventoryView->AddAnim(mHeadView);
    mMainInventoryView->AddAnim(mFaceView);
    mMainInventoryView->AddAnim(mDetailsView);
    mMainInventoryView->AddAnim(mLogosView);
    mMainInventoryView->AddAnim(mEditView);

    mUnknownC8 = kMinimumRowCount;
    mBodyRowCount = kMinimumRowCount;
    mFaceRowCount = kMinimumRowCount;
    mHeadRowCount = kMinimumRowCount;
    mDetailsRowCount = kMinimumRowCount;
    mLogosRowCount = kMinimumRowCount;
    mCurrentRowCount = kMinimumRowCount;
    mEditMeshes.resize(0); // Yes, the binary resizes the vector it has just constructed empty.
    mPaletteColumn = kPaletteStartColumn;
    mPaletteRow = kPaletteStartRow;
    mGridRow = 0;
    mGridColumn = 0;
}

// 0x0026c230
MetFreqMakerInventoryScreen::~MetFreqMakerInventoryScreen() {
    delete mMainInventoryView;
    delete mBodyView;
    delete mHeadView;
    delete mFaceView;
    delete mDetailsView;
    delete mLogosView;
    delete mEditView;
    for (std::list<Rnd::Object *>::iterator it = mPartMeshes.begin(); it != mPartMeshes.end();
         ++it) {
        delete *it;
    }
    mPartMeshes.clear();
    mViewsResolved = 0;
    mCanvas = nullptr;
    mFaceNames.clear();
    mHeadNames.clear();
    mBodyNames.clear();
    mDetailsNames.clear();
    mLogosNames.clear();
    mUnknown58 = kIdleUnknown58;
    mMode = kModeNone;
    mCrossOrigin = nullptr;
}

// 0x00272440
MetFreqMakerInventoryScreen *MetFreqMakerInventoryScreen::New(MetRenderer *pRenderer,
                                                              int nPriority) {
    return new MetFreqMakerInventoryScreen(pRenderer, nPriority);
}

// 0x00272528
void MetFreqMakerInventoryScreen::EnterAndShow() {
    MetScreen::EnterAndShow();
    mMode = kModeNone;
    mUnknown58 = kIdleUnknown58;
}

// 0x0026a3b0
int MetFreqMakerInventoryScreen::PollContainerLoad() {
    bool loaded = MetFreqMakerAssetManager::shared()->PollLoad();
    MetScreen *pDirections = FindScreenByName(HxStr(kDirectionsScreenName));
    loaded = (pDirections->PollContainerLoad() != 0) && loaded;
    if (!loaded) {
        return 0;
    }
    return MetScreen::PollContainerLoad();
}

// 0x00272b78
void MetFreqMakerInventoryScreen::PlaySlideSound(int nSelector) {
    if (mMode == kModeInventory || mMode == kModeColor || mMode == kModePart) {
        MetScreen::PlaySlideSound(nSelector);
    }
}

// 0x00272bb8
void MetFreqMakerInventoryScreen::PlayLeaveSound() {
    if (mMode == kModeInventory || mMode == kModeColor || mMode == kModePart) {
        MetScreen::PlayLeaveSound();
    }
}

// 0x00272c38
void MetFreqMakerInventoryScreen::PlayHighSound(int) {
    if (mMode == kModeInventory) {
        PlaySoundByName(kPartSelectSound);
    }
}

// 0x00272c68
void MetFreqMakerInventoryScreen::PlayCycleLeftSound(int) {
    if (mMode == kModeInventory) {
        PlaySoundByName(kPartSelectSound);
    }
}

// 0x00272c98
void MetFreqMakerInventoryScreen::PlayCycleRightSound(int) {
    if (mMode == kModeInventory) {
        PlaySoundByName(kPartSelectSound);
    }
}

// 0x00272560
void MetFreqMakerInventoryScreen::OnUnknownSlot30(Rnd::Object *) {
    ActivateNamedPanel(HxStr(kPanelName));
}

// 0x00272cc8
void MetFreqMakerInventoryScreen::PlayMoveSound() {
    if (mMode == kModeColor) {
        PlaySoundByName(kColorMoveSound);
    }
    if (mMode == kModePart) {
        PlaySoundByName(kPartMoveSound);
    }
}

// 0x00272bf8
void MetFreqMakerInventoryScreen::PlayFlipSound() {
    PlaySoundByName(kFlipSound);
}

// 0x00272c18
void MetFreqMakerInventoryScreen::PlayModeToggleSound() {
    PlaySoundByName(kToggleSound);
}

// 0x00272b38
void MetFreqMakerInventoryScreen::PlayToggleSound() {
    PlaySoundByName(kToggleSound);
}

// 0x00272b58
void MetFreqMakerInventoryScreen::PlayDeleteSound() {
    PlaySoundByName(kDeleteSound);
}

// 0x00272868
void MetFreqMakerInventoryScreen::GetPalettePosition(Vector2 &position) {
    position.y = static_cast<float>(mPaletteRow) * kPaletteCellHeight + kPaletteCellHeight / 2;
    position.x = static_cast<float>(mPaletteColumn) * kPaletteCellWidth + kPaletteCellWidth / 2;
}

// 0x00272918
void MetFreqMakerInventoryScreen::MoveCrossOrigin(const Vector2 &position) {
    const float translation[] = {position.x * kCrossOriginWidth,
                                 kCrossOriginDepth,
                                 -position.y * kCrossOriginHeight,
                                 kXfmTranslationW};
    std::copy(translation,
              translation + Rnd::kXfmRowFloatCount,
              mCrossOrigin->mLocalXfm[kXfmTranslationRow]);
    mCrossOrigin->mDirty = 1;
}

// 0x00272800
void MetFreqMakerInventoryScreen::MoveCrossOrigin(float flX, float flY) {
    Vector2 position{flX, flY};
    MoveCrossOrigin(position);
}

// 0x00272828
void MetFreqMakerInventoryScreen::UpdateCrossOrigin() {
    Vector2 position{0, 0};
    GetPalettePosition(position);
    MoveCrossOrigin(position);
}

// 0x002727d0
Color *MetFreqMakerInventoryScreen::PaletteColorAt(const Vector2 &position) {
    return MetFreqMakerAssetManager::shared()->ColorAt(position);
}

// 0x002724c8
void MetFreqMakerInventoryScreen::ApplyPaletteColor(Color &color) {
    Vector2 position{0, 0};
    GetPalettePosition(position);
    color = *PaletteColorAt(position);
    mCanvas->SetColor(color, position);
}

// 0x00272988
void MetFreqMakerInventoryScreen::SetPaletteFromPart(FreqPart *pPart) {
    Vector2 position;
    if (pPart->mPalettePosition.x == kUnsetPalettePosition ||
        pPart->mPalettePosition.y == kUnsetPalettePosition) {
        position.x = kPaletteCentre;
        position.y = kPaletteCentre;
    } else {
        position = pPart->mPalettePosition;
    }
    mPaletteColumn = static_cast<int>(position.x * kPaletteColumnCount);
    mPaletteRow = static_cast<int>(position.y * kPaletteRowCount);
    if (mPaletteColumn >= kPaletteColumnCount) {
        mPaletteColumn = kPaletteColumnCount - 1;
    }
    if (mPaletteRow >= kPaletteRowCount) {
        mPaletteRow = kPaletteRowCount - 1;
    }
    Vector2 cursor{0, 0};
    GetPalettePosition(cursor);
    MoveCrossOrigin(cursor);
}

// 0x00272a68
FreqPart *MetFreqMakerInventoryScreen::SelectCurrentPart() {
    return mCanvas->SelectPart(mGridRow * kGridColumnCount + mGridColumn);
}

// 0x00272790
void MetFreqMakerInventoryScreen::DeleteCurrentPart() {
    mCanvas->DeletePart(mGridRow * kGridColumnCount + mGridColumn);
}

// 0x002727c0
void MetFreqMakerInventoryScreen::OnGridTopReached() {
}

// 0x002727c8
void MetFreqMakerInventoryScreen::OnGridBottomReached() {
}

// 0x00272d20
float MetFreqMakerInventoryScreen::PartScale(FreqPart *pPart) {
    switch (pPart->mTemplate->mCategory) {
    case kPartCategoryBody:
    case kPartCategoryHead:
    case kPartCategoryDetails:
    case kPartCategoryDetailsSecond:
    case kPartCategoryDetailsThird:
    case kPartCategoryDetailsFourth:
    case kPartCategoryDetailsFifth:
        return kSmallPartScale;
    case kPartCategoryFaceFirst:
    case kPartCategoryFaceSecond:
    case kPartCategoryFaceThird:
    case kPartCategoryLogos:
        return kLargePartScale;
    default:
        return kUnknownPartScale;
    }
}
