#include "met/metfreqmakerinventoryscreen.h"

#include <algorithm>
#include <map>

#include "app/playsound.h"
#include "game/freqpart.h"
#include "game/freqparttemplate.h"
#include "math/color.h"
#include "math/vector2.h"
#include "met/metfreqmakerassetmanager.h"
#include "met/metfreqmakercanvasscreen.h"
#include "met/metfreqmakerdirectionsscreen.h"
#include "os/formatstring.h"
#include "rnd/animatable.h"
#include "rnd/collideable.h"
#include "rnd/drawable.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
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
static const char *const kCanvasScreenName = "MetFreqMakerCanvasScreen";
static const char *const kPanelName = "MetFreqMakerInventoryScreen";
static const char *const kButtonsScreenName = "MetFreqMakerButtonsScreen";

static const char *const kHeadHeading = "HEAD";
static const char *const kFaceHeading = "FACE";
static const char *const kBodyHeading = "BODY";
static const char *const kDetailsHeading = "DETAILS";
static const char *const kLogosHeading = "LOGOS";
static const char *const kEditHeading = "FreQ";
static const char *const kEditMeshFormat = "editableMesh%i";

static const char *const kMeshSuffix = ".mesh";
static const char *const kGridViewName = "fm_grid.view";
static const char *const kWire16Name = "fm_wire_16.mesh";
static const char *const kWire30Name = "fm_wire_30.mesh";
static const char *const kLimitTextName = "fminv_limit.txt";
static const char *const kCrossOriginName = "fm_cross_origin.view";
static const char *const kSpectrumViewName = "fm_spectrum.view";
static const char *const kColorTextName = "COLOR.txt";
static const char *const kHighlightMeshName = "fm_inventory_hisquare.mesh";
static const char *const kHeadingItemName = "HEADING_ITEM.txt";
static const char *const kHeadingNameName = "HEADING_NAME.txt";
static const char *const kCanvasMeshName = "canvas_2.mesh";
static const char *const kInventoryMeshName = "fm_inventory.mesh";
static const char *const kCanvasHighlightMaterial = "fm_canvas_hi.mat";
static const char *const kCanvasLiveMaterial = "fm_canvas_live.mat";
static const char *const kInventoryMaterial = "fm_inventory.mat";
static const char *const kInventoryHighlightMaterial = "fm_inventory_hi.mat";

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

// The further factors slot 38 scales a part's x and z rows by, and the pitch and height of the
// part grid.
constexpr float kPartWidthFactor = 38.65f;
constexpr float kPartDepthFactor = 38.5f;
constexpr float kGridPitchX = 46.65f;
constexpr float kGridPitchZ = -46.5f;
constexpr float kGridHeight = -4.0f;

// The mapping from a palette position to the translation of `fm_cross_origin.view`.
constexpr int kXfmTranslationRow = 3;
constexpr float kCrossOriginWidth = 375.0f;
constexpr float kCrossOriginHeight = 140.0f;
constexpr float kCrossOriginDepth = -0.1f;
constexpr float kXfmTranslationW = 1.0f;

// MetScreen::mUnknown58 in each state of the screen.
constexpr float kConstructedUnknown58 = 3.0f;
constexpr float kIdleUnknown58 = 1.0f;
constexpr float kBrowsingUnknown58 = 0.5f;
constexpr float kPlacingUnknown58 = 0.02f;

// The commands slot 19 handles beyond the MetScreenCommandCode values. The codes are named
// nowhere in the image, and the names are inferred from the canvas command each forwards.
enum InventoryCommand {
    kInventoryCommandToggleMode = 7,
    kInventoryCommandDelete = 8,
    kInventoryCommandRecentre = 12,
    kInventoryCommandBringForward = 13,
    kInventoryCommandSendBackward = 14,
    kInventoryCommandLeft = 16,
    kInventoryCommandRight = 17,
    kInventoryCommandDown = 18,
    kInventoryCommandUp = 19,
    kInventoryCommandMirror = 21
};

// The MetFreqMakerDirectionsScreen pages the inventory shows.
enum DirectionsPage {
    kDirectionsSelectStamp = 0,
    kDirectionsCanvas = 1,
    kDirectionsColorPanelShort = 2,
    kDirectionsColorPanel = 3,
    kDirectionsFreqFull = 15
};

// The most parts the canvas accepts.
constexpr int kMaxParts = 16;
// The rows of the grid that show at once, on the edit page and on the other pages.
constexpr int kEditVisibleRowCount = 2;
constexpr int kVisibleRowCount = 3;
// The Rnd::Object::Copy() flags ShowEditPage() copies a part's mesh with.
constexpr unsigned kCopyNothing = 0;

// One cell of the part grid.
struct GridCell {
    GridCell(int nColumn, int nRow) : mColumn(nColumn), mRow(nRow) {
    }

    int mColumn;
    int mRow;
};

// 0x00891af8
// The corners of the three visible rows of the grid, through 0x00891b17. No routine reads them.
// The names are inferred.
GridCell g_gridTopLeft(0, 0);
GridCell g_gridBottomLeft(0, 2);
GridCell g_gridTopRight(7, 0);
GridCell g_gridBottomRight(7, 2);

// 0x00272268
// Negate a value unless it is already negative. The product is taken in double precision. The
// routine is never called.
inline void ForceNonPositive(float &flValue) {
    if (!(flValue < 0.0f)) {
        flValue = static_cast<float>(static_cast<double>(flValue) * -1.0);
    }
}

// 0x002722c8
// Negate a value when it is negative. The product is taken in double precision. The routine is
// never called.
inline void ForceNonNegative(float &flValue) {
    if (flValue < 0.0f) {
        flValue = static_cast<float>(static_cast<double>(flValue) * -1.0);
    }
}

// The rows a page of nParts parts needs, never fewer than the visible rows.
inline int PageRowCount(int nParts) {
    int nRows = static_cast<int>(static_cast<float>(nParts) * (1.0f / kGridColumnCount));
    if (nParts % kGridColumnCount > 0) {
        ++nRows;
    }
    if (nRows < kMinimumRowCount) {
        nRows = kMinimumRowCount;
    }
    return nRows;
}

// Resolve one named object of the renderer as T.
template <class T>
inline T *FindObject(const char *pszName) {
    return dynamic_cast<T *>(Rnd::g_manager.Find(HxStr(pszName)));
}

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

    mEditRowCount = kMinimumRowCount;
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
void MetFreqMakerInventoryScreen::PlayLeaveSound(int nSelector) {
    if (mMode == kModeInventory || mMode == kModeColor || mMode == kModePart) {
        MetScreen::PlayLeaveSound(nSelector);
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
void MetFreqMakerInventoryScreen::OnUnknownSlot30(Rnd::Button *) {
    ActivateNamedPanel(HxStr(kPanelName));
}

// 0x0026b518
void MetFreqMakerInventoryScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    std::map<HxStr, FreqPartTemplate *> parts(
        *MetFreqMakerAssetManager::shared()->GetPartsByName());
    HxStr name;
    float flScale = kUnknownPartScale;
    for (std::map<HxStr, FreqPartTemplate *>::iterator it = parts.begin(); it != parts.end();
         ++it) {
        FreqPartTemplate *pTemplate = it->second;
        name = pTemplate->mName + kMeshSuffix;
        Rnd::Mesh *pMesh = MetFreqMakerAssetManager::shared()->CloneMesh(name);
        mPartMeshes.push_back(pMesh);
        pMesh->SetMaterial(pTemplate->mMaterial);
        float flScaleX;
        float flScaleZ;
        MetFreqMakerAssetManager::shared()->ApplyPartScale(
            pMesh, pTemplate, &flScaleX, &flScaleZ, kPartWidthFactor, kPartDepthFactor);

        Rnd::View *pView = nullptr;
        std::vector<HxStr> *pNames = nullptr;
        // Yes, the counts restart for every template. Every page receives the minimum rows.
        int nHeadParts = 0;
        int nBodyParts = 0;
        int nFaceParts = 0;
        int nDetailsParts = 0;
        int nLogosParts = 0;
        switch (pTemplate->mCategory) {
        case kPartCategoryBody:
            pView = mBodyView;
            pNames = &mBodyNames;
            flScale = kSmallPartScale;
            ++nBodyParts;
            break;
        case kPartCategoryHead:
            pView = mHeadView;
            pNames = &mHeadNames;
            flScale = kSmallPartScale;
            ++nHeadParts;
            break;
        case kPartCategoryFaceFirst:
        case kPartCategoryFaceSecond:
        case kPartCategoryFaceThird:
            pView = mFaceView;
            pNames = &mFaceNames;
            flScale = kLargePartScale;
            ++nFaceParts;
            break;
        case kPartCategoryDetails:
        case kPartCategoryDetailsSecond:
        case kPartCategoryDetailsThird:
        case kPartCategoryDetailsFourth:
        case kPartCategoryDetailsFifth:
            pView = mDetailsView;
            pNames = &mDetailsNames;
            flScale = kSmallPartScale;
            ++nDetailsParts;
            break;
        case kPartCategoryLogos:
            pView = mLogosView;
            pNames = &mLogosNames;
            flScale = kLargePartScale;
            ++nLogosParts;
            break;
        default:
            break;
        }
        mHeadRowCount = PageRowCount(nHeadParts);
        mLogosRowCount = PageRowCount(nLogosParts);
        mBodyRowCount = PageRowCount(nBodyParts);
        mDetailsRowCount = PageRowCount(nDetailsParts);
        mFaceRowCount = PageRowCount(nFaceParts);

        int nIndex = pView->GetDraws().size();
        pNames->push_back(pTemplate->mName);
        const float translation[] = {(nIndex % kGridColumnCount) * kGridPitchX,
                                     kGridHeight,
                                     (nIndex / kGridColumnCount) * kGridPitchZ,
                                     kXfmTranslationW};
        std::copy(translation,
                  translation + Rnd::kXfmRowFloatCount,
                  pMesh->mLocalXfm[kXfmTranslationRow]);
        pMesh->mDirty = 1;
        pMesh->ScaleUniform(flScale);
        pView->AddDraw(pMesh, nullptr);
        pView->AddTrans(pMesh);
        pView->AddCollide(pMesh);
    }

    Rnd::View *pGrid = FindObject<Rnd::View>(kGridViewName);
    pGrid->AddTrans(mMainInventoryView);
    pGrid->AddCollide(mMainInventoryView);
    pGrid->AddDraw(mMainInventoryView, nullptr);
    mMainInventoryView->SetShowing(1);
    mViewsResolved = 1;
    mCanvas = static_cast<MetFreqMakerCanvasScreen *>(FindScreenByName(HxStr(kCanvasScreenName)));
    mWire16 = FindObject<Rnd::Mesh>(kWire16Name);
    mWire30 = FindObject<Rnd::Mesh>(kWire30Name);
    mLimitText = FindObject<Rnd::Text>(kLimitTextName);
    mCrossOrigin = FindObject<Rnd::View>(kCrossOriginName);
    ShowPalette(0);
    ShowInventory(0);
    SetHighlight(kHighlightNone);
    mPaletteColumn = 0;
    mPaletteRow = 0;
    UpdateCrossOrigin();
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

// 0x00272600
void MetFreqMakerInventoryScreen::OnUnknownSlot7() {
    mMode = kModeInventory;
    mUnknown58 = kBrowsingUnknown58;
    ShowPalette(0);
    ShowInventory(1);
    SetHighlight(kHighlightInventory);
    mGridRow = 0;
    mGridColumn = 0;
    UpdateGridCursor();
    if (mCurrentView == mEditView) {
        FreqPart *pPart = SelectCurrentPart();
        if (pPart != nullptr) {
            SetPaletteFromPart(pPart);
        }
        ShowDirections(kDirectionsSelectStamp);
    } else {
        Vector2 position{0, 0};
        GetPalettePosition(position);
        Color color = *PaletteColorAt(position);
        mCanvas->SetColor(color, position);
        PreviewCurrentTemplate();
        ShowDirections(kDirectionsSelectStamp);
    }
}

// Expanded into slot 19 for each palette move.
inline void MetFreqMakerInventoryScreen::ApplyPaletteToCurrentMesh() {
    Color color;
    ApplyPaletteColor(color);
    UpdateCrossOrigin();
    if (mCurrentView == mEditView) {
        int nIndex = mGridRow * kGridColumnCount + mGridColumn;
        if (nIndex >= 0 && static_cast<unsigned>(nIndex) < mEditMeshes.size()) {
            mEditMeshes[nIndex]->SetVertexColor(color);
        }
    }
}

// 0x0026c928
void MetFreqMakerInventoryScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandPrevious:
        if (mMode == kModeColor || mMode == kModePart) {
            return;
        }
        if (mGridRow == 0) {
            OnGridTopReached();
            return;
        }
        --mGridRow;
        UpdateGridCursor();
        break;
    case kMetScreenCommandNext:
        if (mMode == kModeColor || mMode == kModePart) {
            return;
        }
        if (mGridRow < mCurrentRowCount) {
            int nVisibleRows =
                (mCurrentView == mEditView) ? kEditVisibleRowCount : kVisibleRowCount;
            if (mGridRow == nVisibleRows - 1) {
                OnGridBottomReached();
                return;
            }
            ++mGridRow;
            UpdateGridCursor();
        }
        break;
    case kMetScreenCommandLeft:
        if (mMode == kModeColor || mMode == kModePart) {
            return;
        }
        if (mGridColumn > 0) {
            --mGridColumn;
            UpdateGridCursor();
        }
        break;
    case kMetScreenCommandRight:
        if (mMode == kModeColor || mMode == kModePart) {
            return;
        }
        if (mGridColumn < kGridColumnCount - 1) {
            ++mGridColumn;
            UpdateGridCursor();
        }
        break;
    case kMetScreenCommandSelect:
        if (mMode <= kModeNone) {
            return;
        }
        if (mMode < kModeInventory) {
            mCanvas->PlaceCursor();
            mMode = kModeNone;
            mUnknown58 = kIdleUnknown58;
            ShowPalette(0);
            ShowInventory(1);
            SetHighlight(kHighlightNone);
            HideGridCursor();
            mGridRow = 0;
            mGridColumn = 0;
            mCanvas->ResetCursor();
            ActivateNamedPanel(HxStr(kButtonsScreenName));
            return;
        }
        if (mMode != kModeInventory || !IsCurrentCellFilled()) {
            return;
        }
        if (mCurrentView != mEditView &&
            static_cast<int>(mCanvas->GetParts().size()) >= kMaxParts) {
            PlayErrorSound(pCommand->mPadIndex);
            return;
        }
        mMode = kModePart;
        mUnknown58 = kPlacingUnknown58;
        SetHighlight(kHighlightCanvas);
        ShowDirections((mCurrentView == mEditView) ? kDirectionsColorPanel :
                                                     kDirectionsColorPanelShort);
        return;
    case kMetScreenCommandBack:
        if (mMode <= kModeNone) {
            return;
        }
        if (mMode < kModeInventory) {
            mMode = kModeInventory;
            mUnknown58 = kBrowsingUnknown58;
            if (mCurrentView == mEditView) {
                mCanvas->RevertSelection();
                ShowEditPage();
            }
            ShowPalette(0);
            ShowInventory(1);
            SetHighlight(kHighlightInventory);
            mCanvas->ResetCursor();
            ShowDirections(kDirectionsSelectStamp);
            break;
        }
        if (mMode != kModeInventory) {
            return;
        }
        ActivateNamedPanel(HxStr(kButtonsScreenName));
        mMode = kModePart; // Yes, leaving the inventory enters the part mode.
        mUnknown58 = kPlacingUnknown58;
        ShowPalette(0);
        ShowInventory(1);
        SetHighlight(kHighlightNone);
        HideGridCursor();
        mGridRow = 0;
        mGridColumn = 0;
        mCanvas->ResetCursor();
        return;
    case kInventoryCommandToggleMode:
        if (mMode == kModeInventory) {
            return;
        }
        mMode = (mMode != kModeColor) ? kModeColor : kModePart;
        PlayModeToggleSound();
        if (mMode == kModePart) {
            ShowPalette(0);
            ShowInventory(1);
            SetHighlight(kHighlightCanvas);
            mUnknown58 = kBrowsingUnknown58;
            ShowDirections((mCurrentView == mEditView) ? kDirectionsColorPanel :
                                                         kDirectionsColorPanelShort);
        } else if (mMode == kModeColor) {
            ShowPalette(1);
            ShowInventory(0);
            SetHighlight(kHighlightInventory);
            mUnknown58 = kBrowsingUnknown58;
            ShowDirections(kDirectionsCanvas);
        }
        return;
    case kInventoryCommandDelete:
        if (mMode != kModePart || mCurrentView != mEditView) {
            return;
        }
        DeleteCurrentPart();
        ShowEditPage();
        UpdateGridCursor();
        mMode = kModeInventory;
        mUnknown58 = kBrowsingUnknown58;
        SetHighlight(kHighlightInventory);
        ShowDirections(kDirectionsSelectStamp);
        PlayDeleteSound();
        return;
    case kInventoryCommandRecentre:
        if (mMode == kModePart && mCurrentView == mEditView) {
            int nIndex = mGridRow * kGridColumnCount + mGridColumn;
            if (nIndex >= 0 && static_cast<unsigned>(nIndex) < mEditMeshes.size()) {
                mCanvas->RecentrePart(nIndex);
                PlayToggleSound();
            }
        }
        return;
    case kInventoryCommandBringForward:
    case kInventoryCommandSendBackward:
        if (mMode == kModePart) {
            mCanvas->HandleCanvasCommand(pCommand);
            PlayFlipSound();
        }
        return;
    case kInventoryCommandLeft:
        if (mMode == kModeInventory) {
            return;
        }
        if (mMode == kModePart) {
            mCanvas->HandleCanvasCommand(pCommand);
            PlayMoveSound();
            return;
        }
        if (mMode != kModeColor) {
            return;
        }
        if (mPaletteColumn > 0) {
            --mPaletteColumn;
            PlayMoveSound();
        }
        ApplyPaletteToCurrentMesh();
        return;
    case kInventoryCommandRight:
        if (mMode == kModeInventory) {
            return;
        }
        if (mMode == kModePart) {
            mCanvas->HandleCanvasCommand(pCommand);
            PlayMoveSound();
            return;
        }
        if (mMode != kModeColor) {
            return;
        }
        if (mPaletteColumn < kPaletteColumnCount - 1) {
            ++mPaletteColumn;
            PlayMoveSound();
        }
        ApplyPaletteToCurrentMesh();
        return;
    case kInventoryCommandDown:
        if (mMode == kModeInventory) {
            return;
        }
        if (mMode == kModePart) {
            mCanvas->HandleCanvasCommand(pCommand);
            PlayMoveSound();
            return;
        }
        if (mMode != kModeColor) {
            return;
        }
        if (mPaletteRow < kPaletteRowCount - 1) {
            ++mPaletteRow;
            PlayMoveSound();
        }
        ApplyPaletteToCurrentMesh();
        return;
    case kInventoryCommandUp:
        if (mMode == kModeInventory) {
            return;
        }
        if (mMode == kModePart) {
            mCanvas->HandleCanvasCommand(pCommand);
            PlayMoveSound();
            return;
        }
        if (mMode != kModeColor) {
            return;
        }
        if (mPaletteRow > 0) {
            --mPaletteRow;
            PlayMoveSound();
        }
        ApplyPaletteToCurrentMesh();
        return;
    case kInventoryCommandMirror:
        if (mMode != kModePart) {
            return;
        }
        mCanvas->HandleCanvasCommand(pCommand);
        PlayFlipSound();
        if (mCurrentView == mEditView) {
            int nIndex = mGridRow * kGridColumnCount + mGridColumn;
            if (nIndex >= 0 && static_cast<unsigned>(nIndex) < mEditMeshes.size()) {
                mEditMeshes[nIndex]->MirrorX();
            }
        }
        return;
    default:
        return;
    }
    if (mCurrentView == mEditView) {
        FreqPart *pPart = SelectCurrentPart();
        if (pPart != nullptr) {
            SetPaletteFromPart(pPart);
        }
    } else {
        PreviewCurrentTemplate();
    }
}

// 0x00272a98
void MetFreqMakerInventoryScreen::OnUnknownSlot33() {
    ShowPalette(0);
    ShowInventory(0);
    mPaletteRow = kPaletteStartRow;
    mPaletteColumn = kPaletteStartColumn;
    Vector2 position{0, 0};
    GetPalettePosition(position);
    Color color = *PaletteColorAt(position);
    mCanvas->SetColor(color, position);
    Vector2 cursor{0, 0};
    GetPalettePosition(cursor);
    MoveCrossOrigin(cursor);
}

// Expanded into each of the five part page routines.
inline void
MetFreqMakerInventoryScreen::ShowPartPage(Rnd::View *pView, int nRowCount, const char *pszHeading) {
    HidePages();
    pView->SetShowing(1);
    mCurrentView = pView;
    mCurrentRowCount = nRowCount;
    FindObject<Rnd::Text>(kHeadingNameName)->SetText(HxStr(pszHeading));
    ShowInventory(1);
    ShowPalette(0);
    HideGridCursor();
    SetHighlight(kHighlightNone);
}

// 0x0026d318
void MetFreqMakerInventoryScreen::ShowHeadPage() {
    ShowPartPage(mHeadView, mHeadRowCount, kHeadHeading);
}

// 0x0026d4c8
void MetFreqMakerInventoryScreen::ShowFacePage() {
    ShowPartPage(mFaceView, mFaceRowCount, kFaceHeading);
}

// 0x0026d678
void MetFreqMakerInventoryScreen::ShowBodyPage() {
    ShowPartPage(mBodyView, mBodyRowCount, kBodyHeading);
}

// 0x0026d828
void MetFreqMakerInventoryScreen::ShowDetailsPage() {
    ShowPartPage(mDetailsView, mDetailsRowCount, kDetailsHeading);
}

// 0x0026d9d8
void MetFreqMakerInventoryScreen::ShowLogosPage() {
    ShowPartPage(mLogosView, mLogosRowCount, kLogosHeading);
}

// 0x0026db88
void MetFreqMakerInventoryScreen::HidePages() {
    mHeadView->SetShowing(0);
    mFaceView->SetShowing(0);
    mBodyView->SetShowing(0);
    mDetailsView->SetShowing(0);
    mLogosView->SetShowing(0);
    mEditView->SetShowing(0);
    mCurrentView = nullptr;
    mCurrentRowCount = -1;
    FindObject<Rnd::Text>(kHeadingNameName)->SetText(HxStr(""));
    ShowInventory(0);
    ShowPalette(0);
    SetHighlight(kHighlightNone);
}

// 0x0026ddc8
void MetFreqMakerInventoryScreen::ShowEditPage() {
    HidePages();
    int nMeshCount = mEditMeshes.size();
    for (int i = 0; i < nMeshCount; ++i) {
        delete mEditMeshes[i];
    }
    mEditMeshes.clear();
    mEditMeshes.resize(0); // Yes, the binary resizes the vector it has just emptied.

    std::list<FreqPart *> &parts = mCanvas->GetParts();
    int nPartCount = parts.size();
    int nIndex = 0;
    for (std::list<FreqPart *>::iterator it = parts.begin(); it != parts.end(); ++it) {
        FreqPart *pPart = *it;
        Rnd::Mesh *pMesh = Rnd::NewMeshThroughHook(HxStr(FormatString(kEditMeshFormat, nIndex)));
        pMesh->Copy(pPart->GetMesh(), kCopyNothing);
        float flScaleX;
        float flScaleZ;
        MetFreqMakerAssetManager::shared()->ApplyPartScale(
            pMesh, pPart->mTemplate, &flScaleX, &flScaleZ, kPartWidthFactor, kPartDepthFactor);
        if (pPart->mMirrored == 1) {
            pMesh->MirrorX();
        }
        pMesh->ScaleUniform(PartScale(pPart));
        const float translation[] = {(nIndex % kGridColumnCount) * kGridPitchX,
                                     kGridHeight,
                                     (nIndex / kGridColumnCount) * kGridPitchZ,
                                     kXfmTranslationW};
        std::copy(translation,
                  translation + Rnd::kXfmRowFloatCount,
                  pMesh->mLocalXfm[kXfmTranslationRow]);
        pMesh->mDirty = 1;
        mEditMeshes.push_back(pMesh);
        mEditView->AddDraw(pMesh, nullptr);
        mEditView->AddTrans(pMesh);
        pMesh->SetShowing(1);
        ++nIndex;
    }

    mEditRowCount = static_cast<int>(static_cast<float>(nPartCount) * (1.0f / kGridColumnCount));
    if (nPartCount % kGridColumnCount > 0) {
        ++mEditRowCount;
    }
    if (mEditRowCount < kVisibleRowCount) {
        mEditRowCount = kEditVisibleRowCount; // Yes, the test and the value differ.
    }
    mEditView->SetShowing(1);
    mCurrentView = mEditView;
    mCurrentRowCount = mEditRowCount;
    ShowInventory(1);
    ShowPalette(0);
    HideGridCursor();
    SetHighlight(kHighlightNone);
    FindObject<Rnd::Text>(kHeadingNameName)->SetText(HxStr(kEditHeading));
}

// 0x0026e448
void MetFreqMakerInventoryScreen::HideGridCursor() {
    FindObject<Rnd::Mesh>(kHighlightMeshName)->SetShowing(0);
}

// 0x0026e528
void MetFreqMakerInventoryScreen::UpdateGridCursor() {
    Rnd::Mesh *pCursor = FindObject<Rnd::Mesh>(kHighlightMeshName);
    const float translation[] = {
        mGridColumn * kGridPitchX, kGridHeight, mGridRow * kGridPitchZ, kXfmTranslationW};
    std::copy(
        translation, translation + Rnd::kXfmRowFloatCount, pCursor->mLocalXfm[kXfmTranslationRow]);
    pCursor->mDirty = 1;
    pCursor->SetShowing(1);
}

// 0x0026eff0
void MetFreqMakerInventoryScreen::ShowDirections(int nPage) {
    MetFreqMakerDirectionsScreen *pDirections =
        static_cast<MetFreqMakerDirectionsScreen *>(FindScreenByName(HxStr(kDirectionsScreenName)));
    int nShown = nPage;
    if (nPage == kDirectionsSelectStamp) {
        if (static_cast<int>(mCanvas->GetParts().size()) >= kMaxParts) {
            nShown = (mCurrentView == mEditView) ? nPage : kDirectionsFreqFull;
        }
    }
    pDirections->ShowPage(nShown);
}

// 0x0026f100
bool MetFreqMakerInventoryScreen::IsCurrentCellFilled() {
    int nIndex = mGridRow * kGridColumnCount + mGridColumn;
    std::vector<HxStr> *pNames = nullptr;
    if (mCurrentView == mBodyView) {
        pNames = &mBodyNames;
    } else if (mCurrentView == mDetailsView) {
        pNames = &mDetailsNames;
    } else if (mCurrentView == mFaceView) {
        pNames = &mFaceNames;
    } else if (mCurrentView == mHeadView) {
        pNames = &mHeadNames;
    } else if (mCurrentView == mLogosView) {
        pNames = &mLogosNames;
    } else if (mCurrentView == mEditView) {
        return mCanvas->SelectPart(nIndex) != nullptr;
    }
    // Yes, with no page shown the binary reads the null list.
    return static_cast<unsigned>(nIndex) < pNames->size();
}

// 0x0026f1b0
std::vector<HxStr> *MetFreqMakerInventoryScreen::GetCurrentPageNames() {
    int nIndex = mGridRow * kGridColumnCount + mGridColumn;
    std::vector<HxStr> *pNames;
    if (mCurrentView == mBodyView) {
        pNames = &mBodyNames;
    } else if (mCurrentView == mDetailsView) {
        pNames = &mDetailsNames;
    } else if (mCurrentView == mFaceView) {
        pNames = &mFaceNames;
    } else if (mCurrentView == mHeadView) {
        pNames = &mHeadNames;
    } else {
        pNames = (mCurrentView == mLogosView) ? &mLogosNames : nullptr;
    }
    // Yes, on the edit page the binary reads the null list.
    return (static_cast<unsigned>(nIndex) < pNames->size()) ? pNames : nullptr;
}

// 0x002726e8
void MetFreqMakerInventoryScreen::PreviewCurrentTemplate() {
    mCanvas->ResetCursor();
    int nIndex = mGridRow * kGridColumnCount + mGridColumn;
    std::vector<HxStr> *pNames = GetCurrentPageNames();
    if (pNames != nullptr) {
        const HxStr &name = (*pNames)[nIndex];
        Vector2 position{0, 0};
        GetPalettePosition(position);
        Color color = *PaletteColorAt(position);
        mCanvas->SetColor(color, position);
        mCanvas->SelectTemplate(name);
    }
}

// 0x0026e9e0
void MetFreqMakerInventoryScreen::ShowPalette(int nShowing) {
    Rnd::View *pSpectrum = FindObject<Rnd::View>(kSpectrumViewName);
    Rnd::Text *pColorText = FindObject<Rnd::Text>(kColorTextName);
    pSpectrum->SetShowing(nShowing);
    mCrossOrigin->SetShowing(nShowing);
    pColorText->SetShowing(nShowing);
}

// 0x0026e690
void MetFreqMakerInventoryScreen::ShowInventory(int nShowing) {
    mLimitText->SetShowing(0);
    mWire16->SetShowing(0);
    mWire30->SetShowing(0);
    if (nShowing != 0) {
        if (mCurrentView == mEditView) {
            mLimitText->SetShowing(1);
            mWire16->SetShowing(1);
        } else {
            mWire30->SetShowing(1);
        }
    }
    Rnd::Mesh *pHighlightMesh = FindObject<Rnd::Mesh>(kHighlightMeshName);
    Rnd::Text *pHeadingItem = FindObject<Rnd::Text>(kHeadingItemName);
    Rnd::Text *pHeadingName = FindObject<Rnd::Text>(kHeadingNameName);
    pHighlightMesh->SetShowing(nShowing);
    pHeadingItem->SetShowing(nShowing);
    pHeadingName->SetShowing(nShowing);
    mMainInventoryView->SetShowing(nShowing);
}

// 0x0026ebb8
void MetFreqMakerInventoryScreen::SetHighlight(int nHighlight) {
    Rnd::Mesh *pCanvasMesh = FindObject<Rnd::Mesh>(kCanvasMeshName);
    HxStr canvasMaterialName("");
    Rnd::Mat *pCanvasMaterial = nullptr;
    Rnd::Mesh *pInventoryMesh = FindObject<Rnd::Mesh>(kInventoryMeshName);
    HxStr inventoryMaterialName("");
    Rnd::Mat *pInventoryMaterial = nullptr;
    switch (nHighlight) {
    case kHighlightCanvas:
        canvasMaterialName = kCanvasHighlightMaterial;
        pCanvasMaterial = dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(canvasMaterialName));
        inventoryMaterialName = kInventoryMaterial;
        pInventoryMaterial = dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(inventoryMaterialName));
        break;
    case kHighlightInventory:
        canvasMaterialName = kCanvasLiveMaterial;
        pCanvasMaterial = dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(canvasMaterialName));
        inventoryMaterialName = kInventoryHighlightMaterial;
        pInventoryMaterial = dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(inventoryMaterialName));
        break;
    case kHighlightNone:
        canvasMaterialName = kCanvasLiveMaterial;
        pCanvasMaterial = dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(canvasMaterialName));
        inventoryMaterialName = kInventoryMaterial;
        pInventoryMaterial = dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(inventoryMaterialName));
        break;
    default:
        break;
    }
    pCanvasMesh->SetMaterial(pCanvasMaterial);
    pInventoryMesh->SetMaterial(pInventoryMaterial);
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
