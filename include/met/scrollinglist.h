#pragma once

#include "met/listdataprovider.h"
#include "rnd/drawable.h"

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
 * This declaration is deliberately partial. It declares no data member. A vector of Rnd::Drawable
 * sits at `+0x80`, a Rnd::Drawable at `+0x24`, and the showing flag at `+0x8c`. The object is 0xa0
 * bytes, which the `operator new` argument at each call site confirms independently of the layout.
 *
 * The constructor takes **eight** parameters and not seven. Seven arrive in a1 through t3 and the
 * eighth arrives on the caller's stack, which the read of `0xc0(sp)` at `0x003fcc00` proves against
 * a frame of exactly 0xc0 bytes with no store to that slot anywhere in the body. All four of t0
 * through t3 are read before they are written. That is the pattern the brief records for the eleven
 * routines in the image that genuinely take a stacked argument.
 *
 * MetFreqMakerDirectionsScreen, MetJukeboxBaseScreen, MetMCFreqDelScreen, and MetRemixLoadScreen
 * all drive one, so the class belongs to the shared front-end widget layer rather than to any one
 * screen.
 */
class ScrollingList {
public:
    /**
     * Build a list over one data provider.
     *
     * Each parameter is recorded by the field it is stored in, because that is the only part of its
     * meaning the image settles. provider lands at `+0x00`, unknown18 at `+0x18`, unknown08 at
     * `+0x08`, unknown20 at `+0x20`, drawable at `+0x24`, unknown28 at `+0x28`, unknown2c at
     * `+0x2c`, and unknown90 at `+0x90`. The scale at `+0x6c`, `+0x3c`, `+0x4c`, and `+0x5c` each
     * take 1.0f, the showing flag at `+0x8c` takes 1, and the remaining words are zeroed.
     *
     * provider is the ListDataProvider, proven from four call sites rather than from the field.
     * Each passes the address of its own ListDataProvider subobject, and the offset tracks that
     * base rather than a fixed field: MetJukeboxEditPlaylistScreen at `0x0022b0d4`,
     * MetFreqMakerDirectionsScreen, and MetRemixLoadScreen all pass `this + 0x8c` and all three
     * place the base at 140, while MetRemixDelScreen at `0x0033acd4` passes `this + 0xe8` and
     * places the base at 232.
     *
     * unknown18 and unknown08 are the two values a jukebox screen stores at its own `+0x90` and
     * `+0x94`, which `0x0022b0b4` and `0x0022b0bc` load in that order. unknown20 is the only
     * parameter the body dereferences, at `0x003fcc18`, where it walks a linked list at `+0x24` of
     * the object to count the rows.
     *
     * The body is not written. It formats a per-row object name, resolves each through the render
     * manager, narrows it with dynamic_cast, and drives six helpers at `0x003fcf20`, `0x003fd038`,
     * `0x003fd150`, `0x003fd268`, `0x00400500`, and `0x00400928` whose signatures are not
     * recovered.
     *
     * @param provider The provider the list reads its rows from.
     * @param unknown18 Stored at `+0x18`.
     * @param unknown08 Stored at `+0x08`.
     * @param unknown20 Stored at `+0x20`, and the row count is read through it.
     * @param drawable Stored at `+0x24`, and setShowing() forwards to it.
     * @param unknown28 Stored at `+0x28`.
     * @param unknown2c Stored at `+0x2c`.
     * @param unknown90 Stored at `+0x90`. The one recovered call site passes zero.
     * @ghidraAddress 0x003fcb00
     */
    ScrollingList(ListDataProvider *provider,
                  int unknown18,
                  int unknown08,
                  int unknown20,
                  Rnd::Drawable *drawable,
                  int unknown28,
                  int unknown2c,
                  int unknown90);

    /**
     * Release every row the list built, then the three containers.
     *
     * Writes the table pointer at `+0x94`, clears `+0x10`, and runs the helper at `0x00401030`. It
     * then repeats the whole per-row teardown once for each of the `+0x08` rows, releasing each
     * row's references through Rnd::Object vtable slot 3 with the tag 3. The vectors at `+0x80` and
     * `+0x74` and the list at `+0x70` are released last. The release under the `__in_chrg` flag is
     * compiler-generated and is not source.
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
     * The inference is confirmed against the body. `0x00401308` writes the argument to `+0x8c`, the
     * field the constructor initialises to 1, and the forward at `0x00401330` is to vtable slot 1
     * of the drawable, which is the same slot MetScreen::SetShowing() forwards to on its view.
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
     * The inference is confirmed against the body. `0x00401378` and `0x0040137c` read the begin and
     * end pointers at `+0x80` and `+0x84`, the stride is 4 bytes, and the forward at `0x004013a4`
     * is to the same drawable vtable slot 1 that setShowing() uses. Unlike setShowing() the routine
     * neither masks the flag nor consults `+0x0c`.
     *
     * @param showing Non-zero to draw each row.
     * @ghidraAddress 0x00401360
     */
    void setEntriesShowing(int showing);
};
