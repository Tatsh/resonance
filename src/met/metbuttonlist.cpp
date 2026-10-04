#include "met/metbuttonlist.h"

#include "os/log.h"
#include "os/mem.h"
#include "rnd/manager.h"

namespace {

// The tag every instance is billed to.
constexpr char kAllocationTag[] = "MetButtonList";

// The sentinel mSelected holds while nothing is selected.
constexpr int kNoSelection = -1;

// Rnd::Button states the list sets and tests.
constexpr int kButtonStateNormal = 0;
constexpr int kButtonStateSelected = 1;
constexpr int kButtonStateDisabled = 3;

} // namespace

void *MetButtonList::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, kAllocationTag);
}

void MetButtonList::operator delete(void *pBlock) {
    OperatorDeleteOverride(pBlock, kAllocationTag);
}

MetButtonList::~MetButtonList() {
    Clear();
}

void MetButtonList::SelectPrevious() {
    const int nStart = mSelected;
    int bDone = 0;
    unsigned nDisabled = 0;
    do {
        int nIndex = mSelected - 1;
        if (!(kNoSelection < nIndex)) {
            nIndex = mButtons.size() - 1;
        }
        mSelected = nIndex;
        if (mButtons[nIndex]->mState == kButtonStateDisabled) {
            ++nDisabled;
        } else {
            bDone = 1;
        }
        if (nDisabled == mButtons.size()) {
            mSelected = nStart;
            bDone = 1;
        }
    } while (!bDone);
    mSelectedButton = mButtons[mSelected];
    ChangeButtonSelected(nStart, mSelected);
}

void MetButtonList::SelectNext() {
    const int nStart = mSelected;
    int bDone = 0;
    unsigned nDisabled = 0;
    do {
        int nIndex = mSelected + 1;
        if (!(nIndex < static_cast<int>(mButtons.size()))) {
            nIndex = 0;
        }
        mSelected = nIndex;
        if (mButtons[nIndex]->mState == kButtonStateDisabled) {
            ++nDisabled;
        } else {
            bDone = 1;
        }
        if (nDisabled == mButtons.size()) {
            mSelected = nStart;
            bDone = 1;
        }
    } while (!bDone);
    mSelectedButton = mButtons[mSelected];
    ChangeButtonSelected(nStart, mSelected);
}

void MetButtonList::ChangeButtonSelected(int nPreviousIndex, int nIndex) {
    if (nPreviousIndex == nIndex) {
        return;
    }
    if (nPreviousIndex != kNoSelection) {
        mButtons[nPreviousIndex]->SetState(kButtonStateNormal);
    }
    mButtons[nIndex]->SetState(kButtonStateSelected);
}

void MetButtonList::Clear() {
    mButtons.erase(mButtons.begin(), mButtons.end());
}

void MetButtonList::Add(const HxStr &objectName, const HxStr &labelText) {
    Rnd::Button *pButton = dynamic_cast<Rnd::Button *>(Rnd::TheManager.Find(objectName));
    if (pButton == nullptr) {
        printf("bad button is %s", objectName.mStr != nullptr ? objectName.mStr : g_szEmptyString);
    }
    Append(pButton, labelText);
}

void MetButtonList::SetSelected(int nIndex) {
    if (mButtons.size() == 0 || nIndex == mSelected) {
        return;
    }
    if (nIndex == kNoSelection) {
        mButtons[mSelected]->SetState(kButtonStateNormal);
    } else if (mSelected != nIndex) {
        ChangeButtonSelected(mSelected, nIndex);
    } else {
        // Yes, the binary keeps this branch, which the test above makes unreachable.
        mButtons[nIndex]->SetState(kButtonStateSelected);
    }
    mSelected = nIndex;
    // Yes, the sentinel path reads one element below the first.
    mSelectedButton = mButtons.begin()[nIndex];
}

Rnd::Button *MetButtonList::GetButton(int nIndex) const {
    if (nIndex == kNoSelection) {
        return nullptr;
    }
    return mButtons[nIndex];
}
