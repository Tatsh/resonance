#pragma once

/**
 * Scrolling list of rows driven by a ListDataProvider.
 *
 * `13ScrollingList` in the RTTI descriptor at `0x0086f6a0`, a leaf class with no base. The class
 * name is the RTTI spelling verbatim. No method name and no member name is attested anywhere in
 * the image, so the two methods below follow the required style rather than a recovered spelling.
 *
 * Following the g++ 2.x layout for a class with no base, the vptr sits after the data members at
 * `+0x94`, and the object is 0xa0 bytes. The two-entry vtable at `0x00815a20` runs the
 * compiler-generated GetTypeInfo at `0x00400e88` and the destructor at `0x003fd380`, so the
 * destructor is the one virtual the class declares. The descriptor is built through the built-in
 * type_info constructor at `0x00478900`, which agrees with the absence of a base.
 *
 * This declaration is deliberately partial. It covers only the two routines the jukebox screens
 * run directly, and it declares no data member. The constructor at `0x003fcb00` takes seven
 * arguments, the first of which is the ListDataProvider the list reads its rows from, and the
 * second and third of which are the two geometry values a jukebox screen stores at `+0x90` and
 * `+0x94` of itself. A vector of Rnd::Drawable sits at `+0x80`, a Rnd::Drawable at `+0x24`, and
 * the showing flag at `+0x8c`.
 *
 * MetFreqMakerDirectionsScreen, MetJukeboxBaseScreen, MetMCFreqDelScreen, and MetRemixLoadScreen
 * all drive one, so the class belongs to the shared front-end widget layer rather than to any one
 * screen.
 */
class ScrollingList {
public:
    /**
     * Release the list.
     *
     * @ghidraAddress 0x003fd380
     */
    virtual ~ScrollingList();

    /**
     * Record the showing flag and pass it to the single drawable at `+0x24`.
     *
     * The drawable receives zero whenever the field at `+0x0c` is clear, and the low bit of
     * showing otherwise. A null drawable is skipped. The method name is inferred from the field the
     * routine writes and from the Rnd::Drawable virtual it forwards to.
     *
     * @param showing Non-zero to draw the list.
     * @ghidraAddress 0x00401300
     */
    void setShowing(int showing);

    /**
     * Pass a showing flag to every drawable of the vector at `+0x80`.
     *
     * The flag is forwarded unchanged and the vector is walked in order. The method name is
     * inferred from the vector the routine walks and from the Rnd::Drawable virtual it forwards to.
     *
     * @param showing Non-zero to draw each row.
     * @ghidraAddress 0x00401360
     */
    void setEntriesShowing(int showing);
};
