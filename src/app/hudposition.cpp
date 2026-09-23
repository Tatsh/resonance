#include "app/hudposition.h"

#include <cstring>

#include "app/hudutil.h"
#include "app/overlay.h"
#include "game/playmap.h"
#include "math/vector3.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/font.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"
#include "rnd/meshanim.h"
#include "rnd/meshvert.h"
#include "rnd/text.h"
#include "rnd/view.h"

namespace {

// Bar the constructor starts with. No song position produces it.
constexpr int kNoBar = -99999;

// Section index that marks no current section.
constexpr int kNoSection = -1;

// Width all the blocks share, and the gap each block gives up to its neighbours, in the frames of
// `HUD pos.msnm`.
constexpr int kPositionWidth = 450;
constexpr float kBlockMargin = 21.0f;

// Vertex of a sized block whose second coordinate is the block's width.
constexpr int kWidthVertex = 7;

// Rows of a local transform.
constexpr int kXfmRowTranslation = 3;

// Horizontal scale of a block before and after the current section, and of the current one.
constexpr float kOtherScale = 1.1f;
constexpr float kCurrentScale = 1.85f;

// A label sits halfway along its block.
constexpr float kLabelPlacement = 0.5f;

// Place a transform row, including the padding word, at one position.
inline void SetTranslation(Rnd::Transformable *pTrans, const Vector3 &position) {
    std::memcpy(pTrans->mLocalXfm[kXfmRowTranslation], &position, sizeof(pTrans->mLocalXfm[0]));
    pTrans->mDirty = 1;
}

// Scale a block across by a factor, replacing the three basis rows of its local transform.
inline void ScaleBlock(Rnd::Mesh *pBlock, float flScale) {
    const Vector3 basis[] = {
        {flScale, 0.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f, 1.0f},
    };
    std::memcpy(pBlock->mLocalXfm, basis, sizeof(basis));
    pBlock->mDirty = 1;
}

} // namespace

// 0x00419f88
HudPosition::HudPosition(PlayMap *pPlayMap)
    : mPlayMap(pPlayMap), mCurrentSection(kNoSection), mBar(kNoBar), mUnknown3c(0) {
    const char *pszLayout =
        g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    mView = dynamic_cast<Rnd::View *>(
        Rnd::g_manager.Find(HxStr(FormatString("%s pos.view", pszLayout))));
    mView->ClearTransList();
    mView->ClearDraws();

    mNormalMat = dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(HxStr("pos_norm.mat")));
    mPastMat = dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(HxStr("pos_past.mat")));
    mCurrentMat = dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(HxStr("pos_current.mat")));
    mBlackFont = dynamic_cast<Rnd::Font *>(Rnd::g_manager.Find(HxStr("HUD pos_black.font")));
    mWhiteFont = dynamic_cast<Rnd::Font *>(Rnd::g_manager.Find(HxStr("HUD pos_white.font")));
    mAnim = dynamic_cast<Rnd::MeshAnim *>(Rnd::g_manager.Find(HxStr("HUD pos.msnm")));
    mRepeatView = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr("HUD pos_repeat.view")));

    mView->SetShowing(1);
    const Vector3 origin{0.0f, 0.0f, 0.0f, 1.0f};
    mView->SetOrigin(&origin.x);

    Rnd::Mesh *pBlockTemplate =
        dynamic_cast<Rnd::Mesh *>(Rnd::g_manager.Find(HxStr("HUD pos_norm.mesh")));
    Rnd::Text *pLabelTemplate =
        dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(HxStr("HUD pos.txt")));

    const int nSections = mPlayMap->Slot10();
    int nStartBar = 0;
    Rnd::Text *pFirstLabel = nullptr;
    float flX = 0.0f;
    const float flFrame = static_cast<float>(kPositionWidth / nSections) - kBlockMargin;
    for (int i = 0; i < nSections; ++i) {
        const int nIndex = mPlayMap->Slot11(i);
        const int nBarCount = mPlayMap->mSectionLengths[nIndex];

        Rnd::Mesh *pBlock = Rnd::NewMeshThroughHook(NextHudName());
        pBlock->Copy(pBlockTemplate, 0);
        mAnim->SetMesh(pBlock);
        mAnim->SetFrame(flFrame);
        const float flBlockWidth = pBlock->mVertsOwner->mVerts[kWidthVertex].mPoint.y;
        SetTranslation(pBlock, Vector3{0.0f, flX, 0.0f, 1.0f});
        mView->AddDraw(pBlock);
        mView->AddTrans(pBlock);

        Rnd::Text *pLabel = Rnd::NewTextThroughHook(NextHudName());
        pLabel->Copy(pLabelTemplate, 0);
        pLabel->SetText(mPlayMap->mSectionNames[nIndex]);
        SetTranslation(pLabel, Vector3{0.0f, flX + flBlockWidth * kLabelPlacement, 0.0f, 1.0f});
        pLabel->SetShowing(0);
        mView->AddDraw(pLabel, nullptr);
        mView->AddTrans(pLabel);

        if (i == 0) {
            pFirstLabel = pLabel;
        }
        const Section section{pBlock, pLabel, nStartBar, nBarCount, flFrame};
        mSections.push_back(section);
        nStartBar += nBarCount;
        flX += flBlockWidth;
    }

    mWidth = flX;
    mView->AddDraw(mRepeatView, pFirstLabel);
    mView->AddTrans(mRepeatView);
}

// 0x0041ac18
HudPosition::~HudPosition() {
    for (std::vector<Section>::iterator it = mSections.begin(); it != mSections.end(); ++it) {
        delete it->mBlock;
        delete it->mLabel;
    }
    const Vector3 origin{0.0f, 0.0f, 0.0f, 1.0f};
    mView->SetOrigin(&origin.x);
}

// 0x0041ad88
void HudPosition::Update() {
    int nCurrent = kNoSection;
    if (mBar >= 0) {
        nCurrent = mPlayMap->Slot12(mBar);
    }

    mCurrentSection = kNoSection;
    for (int i = 0; i < static_cast<int>(mSections.size()); ++i) {
        Section &section = mSections[i];
        if (i < nCurrent) {
            ScaleBlock(section.mBlock, kOtherScale);
            section.mBlock->SetMaterial(mPastMat);
            section.mLabel->SetFont(mWhiteFont);
        } else if (i == nCurrent) {
            ScaleBlock(section.mBlock, kCurrentScale);
            section.mBlock->SetMaterial(mCurrentMat);
            section.mLabel->SetFont(mBlackFont);
            mCurrentSection = i;
        } else {
            ScaleBlock(section.mBlock, kOtherScale);
            section.mBlock->SetMaterial(mNormalMat);
            section.mLabel->SetFont(mWhiteFont);
        }
    }

    if (mCurrentSection != kNoSection && mPlayMap->Slot14(mBar) != 0) {
        const Section &current = mSections[mCurrentSection];
        std::memcpy(mRepeatView->mLocalXfm[kXfmRowTranslation],
                    current.mBlock->mLocalXfm[kXfmRowTranslation],
                    sizeof(mRepeatView->mLocalXfm[0]));
        mRepeatView->mDirty = 1;
        mRepeatView->SetFrame(current.mFrame);
        mRepeatView->SetShowing(1);
    } else {
        mRepeatView->SetShowing(0);
    }
}
