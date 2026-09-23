#include "met/scrollinglist.h"

#include <cstring>

#include "os/formatstring.h"
#include "os/hxstr.h"
#include "os/zone.h"
#include "rnd/collectchildren.h"
#include "rnd/drawable.h"
#include "rnd/manager.h"
#include "rnd/mesh.h"
#include "rnd/object.h"
#include "rnd/text.h"
#include "rnd/view.h"

namespace {

// The prefix each row clone's name takes before the template's name.
static const char *const kRowNameFormat = "v_%02d_";

// The copy flags makeRow() clones the template with.
constexpr unsigned kRowCloneFlags = 0x200;

// The class names buildRowCells() sorts a row's children by.
static const char *const kTextClassName = "Text";
static const char *const kMeshClassName = "Mesh";

// The transform row that holds the translation, and the component rows are spaced along.
constexpr int kTranslationRow = Rnd::kXfmRowCount - 1;
constexpr int kRowAxis = 2;

} // namespace

// 0x003fcb00
ScrollingList::ScrollingList(ListDataProvider *pProvider,
                             int nRowPitch,
                             int nRowCount,
                             Rnd::View *pTemplate,
                             Rnd::Mesh *pHighlight,
                             Rnd::Mesh *pUpArrow,
                             Rnd::Mesh *pDownArrow,
                             int nContext)
    : mProvider(pProvider), mUnknown04(0), mRowCount(nRowCount), mItemCount(0), mCursorRow(0),
      mSelected(0), mRowPitch(nRowPitch), mTemplateDrawCount(0), mTemplate(pTemplate),
      mHighlight(pHighlight), mUpArrow(pUpArrow), mDownArrow(pDownArrow), mShowing(1),
      mContext(nContext) {
    // The padding word of each transform row starts at 1.0f, as every Vector3 does.
    for (int i = 0; i < Rnd::kXfmRowCount; ++i) {
        mHighlightXfm[i][Rnd::kXfmRowFloatCount - 1] = 1.0f;
    }

    int nZone = ZoneGetCurrent();
    ZoneSetCurrent(kNoZone);

    std::list<Rnd::Drawable *> &templateDraws = mTemplate->GetDraws();
    mTemplateDrawCount = templateDraws.size();
    for (std::list<Rnd::Drawable *>::iterator it = templateDraws.begin(); it != templateDraws.end();
         ++it) {
        (*it)->SetShowing(1);
    }

    mRows.resize(mRowCount, nullptr);
    for (int i = 0; i < mRowCount; ++i) {
        Rnd::View *pRow = makeRow(i);
        float translation[Rnd::kXfmRowFloatCount];
        std::memcpy(translation, mTemplate->mLocalXfm[kTranslationRow], sizeof(translation));
        translation[kRowAxis] -= mRowPitch * i;
        std::memcpy(pRow->mLocalXfm[kTranslationRow], translation, sizeof(translation));
        pRow->mDirty = 1;
        buildRowCells(pRow);
        mRows[i] = pRow;
    }

    mTemplate->SetShowing(0);
    if (mHighlight != nullptr) {
        std::memcpy(mHighlightXfm, mHighlight->mLocalXfm, sizeof(mHighlightXfm));
    }
    ZoneSetCurrent(nZone);
}

// 0x003fd380
ScrollingList::~ScrollingList() {
    HxStr templateName(mTemplate->mName);
    mCursorRow = 0;
    updateHighlight();

    for (int i = 0; i < mRowCount; ++i) {
        HxStr prefix(FormatString(kRowNameFormat, i));
        HxStr name = prefix + templateName;
        Rnd::Object *pObject = Rnd::g_manager.Find(name);
        Rnd::View *pRow = pObject != nullptr ? dynamic_cast<Rnd::View *>(pObject) : nullptr;

        std::list<Rnd::Object *> objects;
        Rnd::CollectChildren(objects, static_cast<Rnd::Animatable *>(pRow));
        Rnd::CollectChildren(objects, static_cast<Rnd::Collideable *>(pRow));
        Rnd::CollectChildren(objects, static_cast<Rnd::Drawable *>(pRow));
        Rnd::CollectChildren(objects, static_cast<Rnd::Transformable *>(pRow));
        objects.sort();
        objects.unique();
        for (std::list<Rnd::Object *>::iterator it = objects.begin(); it != objects.end(); ++it) {
            delete *it;
        }
        delete static_cast<Rnd::Object *>(pRow);
    }
}

// 0x003fdec0
Rnd::View *ScrollingList::makeRow(int nIndex) {
    HxStr prefix(FormatString(kRowNameFormat, nIndex));
    Rnd::Object *pObject =
        Rnd::g_manager.ResolveAndLinkObject(mTemplate, prefix, kRowCloneFlags, 1, 1);
    return pObject != nullptr ? dynamic_cast<Rnd::View *>(pObject) : nullptr;
}

// 0x003fd858
void ScrollingList::buildRowCells(Rnd::View *pRow) {
    std::vector<Cell> cells;
    std::list<Rnd::Drawable *> &draws = pRow->GetDraws();
    for (std::list<Rnd::Drawable *>::iterator it = draws.begin(); it != draws.end(); ++it) {
        Rnd::Drawable *pDraw = *it;
        if (pDraw->ClassName() == kTextClassName) {
            cells.push_back(Cell{static_cast<Rnd::Text *>(pDraw), nullptr});
            mTextCells.push_back(pDraw);
        } else if (pDraw->ClassName() == kMeshClassName) {
            cells.push_back(Cell{nullptr, static_cast<Rnd::Mesh *>(pDraw)});
        }
    }
    mRowCells.push_back(cells);
}

// 0x003fdd18
void ScrollingList::refresh() {
    int nItem = mSelected - mCursorRow > -1 ? mSelected - mCursorRow : 0;
    int nRow = 0;
    for (std::list<std::vector<Cell>>::iterator row = mRowCells.begin(); row != mRowCells.end();
         ++row) {
        if (nRow < mItemCount) {
            mRows[nRow]->SetShowing(1);
            int nColumn = 0;
            for (std::vector<Cell>::iterator cell = row->begin(); cell != row->end();
                 ++cell, ++nColumn) {
                if (cell->mMesh != nullptr) {
                    mProvider->ProvideMesh(nItem, nColumn, cell->mMesh, mContext);
                } else if (cell->mText != nullptr) {
                    mProvider->ProvideText(nItem, nColumn, cell->mText, mContext);
                }
            }
        } else {
            mRows[nRow]->SetShowing(0);
        }
        ++nItem;
        ++nRow;
    }
    updateArrows();
}

// 0x00400ec8
void ScrollingList::scrollUp() {
    if (mItemCount == 0) {
        if (mHighlight != nullptr) {
            mHighlight->SetShowing(0);
        }
        return;
    }
    if (mHighlight != nullptr) {
        mHighlight->SetShowing(mShowing & 1);
    }
    if (mSelected > 0) {
        if (mCursorRow > 0) {
            --mCursorRow;
        }
        --mSelected;
    }
    updateHighlight();
    refresh();
}

// 0x00400f78
void ScrollingList::scrollDown() {
    // Unlike scrollUp(), the highlight is neither tested for null nor masked with mShowing.
    if (mItemCount == 0) {
        mHighlight->SetShowing(0);
        return;
    }
    mHighlight->SetShowing(1);
    if (mSelected < mItemCount - 1) {
        if (mCursorRow < mRowCount - 1) {
            ++mCursorRow;
        }
        ++mSelected;
    }
    updateHighlight();
    refresh();
}

// 0x00401030
void ScrollingList::updateHighlight() {
    if (mHighlight == nullptr) {
        return;
    }
    float translation[Rnd::kXfmRowFloatCount];
    std::memcpy(translation, mHighlightXfm[kTranslationRow], sizeof(translation));
    translation[kRowAxis] -= mCursorRow * mRowPitch;
    std::memcpy(mHighlight->mLocalXfm[kTranslationRow], translation, sizeof(translation));
    mHighlight->mDirty = 1;
}

// 0x00401088
void ScrollingList::setItemCount(int nItemCount) {
    mItemCount = nItemCount;
    if (nItemCount <= 0) {
        if (mHighlight != nullptr) {
            mHighlight->SetShowing(0);
        }
        return;
    }
    if (mHighlight != nullptr) {
        mHighlight->SetShowing(mShowing & 1);
    }
    if (mSelected >= mItemCount) {
        mSelected = mItemCount - 1;
    }
    updateHighlight();
}

// 0x00401160
int ScrollingList::getSelected() {
    return mSelected;
}

// 0x00401168
void ScrollingList::setSelected(int nSelected) {
    if (mItemCount == 0) {
        mSelected = 0;
        mCursorRow = 0;
        updateHighlight();
        return;
    }
    mSelected = nSelected;
    if (mRowCount < mItemCount && mItemCount - mRowCount < nSelected) {
        mCursorRow = nSelected - (mItemCount - mRowCount);
    } else {
        mCursorRow = nSelected;
    }
    updateHighlight();
}

// 0x00401270
void ScrollingList::updateArrows() {
    if (mUpArrow != nullptr) {
        mUpArrow->SetShowing(0 < mSelected - mCursorRow);
    }
    if (mDownArrow != nullptr) {
        mDownArrow->SetShowing(mSelected - mCursorRow + mRowCount < mItemCount);
    }
}

// 0x00401300
void ScrollingList::setShowing(int nShowing) {
    mShowing = nShowing;
    if (mHighlight == nullptr) {
        return;
    }
    mHighlight->SetShowing(mItemCount == 0 ? 0 : nShowing & 1);
}

// 0x00401360
void ScrollingList::setEntriesShowing(int nShowing) {
    for (std::vector<Rnd::Drawable *>::iterator it = mTextCells.begin(); it != mTextCells.end();
         ++it) {
        (*it)->SetShowing(nShowing);
    }
}
