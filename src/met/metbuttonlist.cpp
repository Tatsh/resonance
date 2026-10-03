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

// NTSC-U/C: 0x001fecc8, PAL: 0x00206198
void *MetButtonList::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, kAllocationTag);
}

// NTSC-U/C: 0x001fece8, PAL: 0x002061b8
void MetButtonList::operator delete(void *pBlock) {
    FreeTaggedMemory(pBlock, kAllocationTag);
}

// NTSC-U/C: 0x001fca30, PAL: 0x00203ed8
MetButtonList::~MetButtonList() {
    Clear();
}

// NTSC-U/C: 0x001fcc40, PAL: 0x002040e8
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
    OnSelectionChanged(nStart, mSelected);
}

// NTSC-U/C: 0x001fcd10, PAL: 0x002041b8
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
    OnSelectionChanged(nStart, mSelected);
}

// NTSC-U/C: 0x001feed8, PAL: 0x002063a8
void MetButtonList::OnSelectionChanged(int nPreviousIndex, int nIndex) {
    if (nPreviousIndex == nIndex) {
        return;
    }
    if (nPreviousIndex != kNoSelection) {
        mButtons[nPreviousIndex]->SetState(kButtonStateNormal);
    }
    mButtons[nIndex]->SetState(kButtonStateSelected);
}

// NTSC-U/C: 0x001fedb0, PAL: 0x00206280
void MetButtonList::Clear() {
    mButtons.erase(mButtons.begin(), mButtons.end());
}

// NTSC-U/C: 0x001fcb28, PAL: 0x00203fd0
void MetButtonList::Add(const HxStr &objectName, const HxStr &labelText) {
    Rnd::Button *pButton = dynamic_cast<Rnd::Button *>(Rnd::g_manager.Find(objectName));
    if (pButton == nullptr) {
        LogPrintf("bad button is %s",
                  objectName.mStr != nullptr ? objectName.mStr : g_szEmptyString);
    }
    Append(pButton, labelText);
}

// NTSC-U/C: 0x001fee08, PAL: 0x002062d8
void MetButtonList::SetSelected(int nIndex) {
    if (mButtons.size() == 0 || nIndex == mSelected) {
        return;
    }
    if (nIndex == kNoSelection) {
        mButtons[mSelected]->SetState(kButtonStateNormal);
    } else if (mSelected != nIndex) {
        OnSelectionChanged(mSelected, nIndex);
    } else {
        // Yes, the binary keeps this branch, which the test above makes unreachable.
        mButtons[nIndex]->SetState(kButtonStateSelected);
    }
    mSelected = nIndex;
    // Yes, the sentinel path reads one element below the first.
    mSelectedButton = mButtons.begin()[nIndex];
}

// NTSC-U/C: 0x001fef40, PAL: 0x00206410
Rnd::Button *MetButtonList::ButtonAt(int nIndex) const {
    if (nIndex == kNoSelection) {
        return nullptr;
    }
    return mButtons[nIndex];
}
