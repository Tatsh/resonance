#pragma once

#include <cstddef>
#include <vector>

#include "os/hxstr.h"
#include "rnd/button.h"
#include "rnd/text.h"

/**
 * Ring of front-end buttons with one of them selected.
 *
 * Its RTTI descriptor is at `0x0086f5f8`. It is a leaf class with no base. Following the g++ 2.x
 * layout for a class with no base, the vptr sits after the data members at `+0x14`, and the object
 * is 0x18 bytes. The five-entry vtable is at `0x007e90f8`.
 *
 * The class supplies its own allocation function, which tags every instance with the literal
 * `MetButtonList` at `0x007e8e10`. That is the one place in the foundation where a Met class
 * declares `operator new`.
 *
 * Both navigation virtuals walk mButtons in their own direction, wrap at the end, and skip an
 * entry whose field at `+0x1c` equals 3, which is how a disabled button is passed over. Both stop
 * after visiting every entry, so a list in which every button is disabled retains the mSelected
 * value it had before the call.
 *
 * The four declared virtuals are the destructor at `0x001fca30`, the two navigation routines at
 * `0x001fcc40` and `0x001fcd10`, and the selection-change notification at `0x001feed8`. The
 * titles of the two navigation routines are inferred from what the bodies do.
 *
 * The constructor at `0x001fc9f8` is inline. It writes the vptr, zeroes the first word and the
 * vector, and sets mSelected to -1, which is the sentinel for no selection that the
 * selection-change routine tests for.
 */
class MetButtonList {
public:
    /**
     * Allocate an instance from the tagged heap.
     *
     * @param nSize The object size, which the compiler supplies.
     * @return The block.
     * @ghidraAddress NTSC-U/C: 0x001fecc8
     * @ghidraAddress PAL: 0x00206198
     */
    void *operator new(size_t nSize);

    /**
     * Release an instance to the tagged heap under the same tag.
     *
     * The out-of-line copy has no caller.
     *
     * @param pBlock The block.
     * @ghidraAddress NTSC-U/C: 0x001fece8
     * @ghidraAddress PAL: 0x002061b8
     */
    void operator delete(void *pBlock);

    /**
     * Construct an empty list with nothing selected.
     *
     * Inline. MetArenasScreen's constructor calls the out-of-line copy.
     *
     * @ghidraAddress NTSC-U/C: 0x001fc9f8
     * @ghidraAddress PAL: 0x00203ea0
     */
    MetButtonList() : mSelectedButton(nullptr), mSelected(-1) {
    }

    /**
     * Release every button reference and the vector.
     *
     * @ghidraAddress NTSC-U/C: 0x001fca30
     * @ghidraAddress PAL: 0x00203ed8
     */
    virtual ~MetButtonList();

    /**
     * Move the selection to the previous enabled button, wrapping from the first to the last.
     *
     * Slot 2. The body steps mSelected down by one. MetMainScreen at `0x002c6820` and
     * MetLoadFreqBaseScreen::HandleCommand() route kMetScreenCommandPrevious here.
     *
     * @ghidraAddress NTSC-U/C: 0x001fcc40
     * @ghidraAddress PAL: 0x002040e8
     */
    virtual void SelectPrevious();

    /**
     * Move the selection to the next enabled button, wrapping from the last to the first.
     *
     * Slot 3. The body steps mSelected up by one, and the screens route kMetScreenCommandNext
     * here.
     *
     * @ghidraAddress NTSC-U/C: 0x001fcd10
     * @ghidraAddress PAL: 0x002041b8
     */
    virtual void SelectNext();

    /**
     * Respond to the selection moving.
     *
     * Slot 4. SetSelected() and both navigation slots call it with the index that was selected
     * and the index that is now selected, in that order. The body puts the previous button back
     * into state 0 and the new one into state 1.
     *
     * @param nPreviousIndex The index the selection had.
     * @param nIndex The index the selection now has.
     * @ghidraAddress NTSC-U/C: 0x001feed8
     * @ghidraAddress PAL: 0x002063a8
     */
    virtual void ChangeButtonSelected(int nPreviousIndex, int nIndex);

    /**
     * Release every button reference and empty mButtons.
     *
     * The body is the vector clear over mButtons, reached as `this + 4`, with the per-element
     * release at `0x00520be0`. Every class with a vector of four-byte elements shares this body.
     *
     * @ghidraAddress NTSC-U/C: 0x001fedb0
     * @ghidraAddress PAL: 0x00206280
     */
    void Clear();

    /**
     * Resolve one button by object name and append it.
     *
     * The object is resolved through Rnd::Manager::Find() on Rnd::TheManager and cast to
     * Rnd::Button. A name that resolves to nothing reports through `0x0053dde0` and the null is
     * appended regardless, which is why every reader tests an entry before using it. A label of
     * length zero is not applied, and every MetLoadFreqBaseScreen call site passes the empty
     * literal so that the label is set later by
     * MetLoadFreqBaseScreen::UpdateNameLabel() instead.
     *
     * @param objectName The button object as written in the `.rnd` file.
     * @param labelText The text for the button's label child, or the empty string for none.
     * @ghidraAddress NTSC-U/C: 0x001fcb28
     * @ghidraAddress PAL: 0x00203fd0
     */
    void Add(const HxStr &objectName, const HxStr &labelText);

    /**
     * Append one resolved button and apply its label.
     *
     * Inline. Add() expands it after resolving the button, and the out-of-line copy has no
     * caller. A label of length zero is not applied. The title is inferred.
     *
     * @param pButton The button, or null.
     * @param labelText The text for the button's label child, or the empty string for none.
     * @ghidraAddress NTSC-U/C: 0x001fed30
     * @ghidraAddress PAL: 0x00206200
     */
    void Append(Rnd::Button *pButton, const HxStr &labelText) {
        mButtons.push_back(pButton);
        if (labelText.mLen != 0) {
            pButton->mText->SetText(labelText);
        }
    }

    /**
     * Move the selection to one index.
     *
     * A call on an empty list does nothing, and so does a call with the index already selected.
     * The sentinel -1 puts the previously selected button back into state 0 and selects nothing.
     * Any other index runs the selection-change notification in vtable slot 4.
     *
     * The routine then stores `mButtons[nIndex]` into mSelectedButton on every path, including the
     * sentinel path, where the index is -1 and the read is one element below the first. That is
     * what the binary does.
     *
     * @param nIndex The index to select, or -1 for none.
     * @ghidraAddress NTSC-U/C: 0x001fee08
     * @ghidraAddress PAL: 0x002062d8
     */
    void SetSelected(int nIndex);

    /**
     * Resolve one button by index.
     *
     * @param nIndex The index, or -1.
     * @return The button, or null for the -1 sentinel. An index past the end is not checked.
     * @ghidraAddress NTSC-U/C: 0x001fef40
     * @ghidraAddress PAL: 0x00206410
     */
    Rnd::Button *GetButton(int nIndex) const;

    /**
     * The button SetSelected() last stored.
     *
     * SetSelected() writes `mButtons[nIndex]` here on every path, including the path that takes
     * the -1 sentinel, where the read is one element below the first.
     *
     * Public rather than private, because MetLoadFreqBaseScreen::HandleCommand() passes it
     * straight to MetScreen::StartRepeatingSound() and the image has no accessor to route that
     * read through. A friend declaration fits the image equally well. +0x00
     */
    Rnd::Button *mSelectedButton;

    /**
     * The buttons in list order.
     *
     * The element is Rnd::Button rather than Rnd::Object. Add() appends the result of a
     * `dynamic_cast` to Rnd::Button, and both navigation virtuals read Rnd::Button::mState at
     * `+0x1c` to pass over a disabled entry.
     *
     * Public rather than private, because MetMsgScreen::Refresh() at `0x002ecd10` reads its size
     * directly and the image has no accessor to route that read through. +0x04
     */
    std::vector<Rnd::Button *> mButtons;

    /**
     * Index of the selected button, or -1 for none.
     *
     * Public rather than private, because MetLoadFreqBaseScreen reads it directly and the image
     * has no accessor to route that read through. A friend declaration fits the image equally
     * well. +0x10
     */
    int mSelected;
};
