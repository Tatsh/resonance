#pragma once

#include <list>

#include "rnd/object.h"

class FailSink;
class MetRenderer;
namespace Rnd {
class Stream;
}

namespace Rnd {

/**
 * Mix-in for an object driven by a frame number.
 *
 * Its RTTI descriptor is at `0x008eed68`. It has `Rnd::Object` as a public virtual base at offset
 * 0. The subobject is 0x18 bytes. The compiler places the virtual-base pointer at `+0x00` and,
 * following the g++ 2.x layout for a class with no non-virtual base, the vptr at `+0x14`. The
 * declared members therefore occupy `+0x04` through `+0x13`. For a standalone Animatable the
 * `Rnd::Object` subobject sits at `+0x18`. The `-0x18` adjustment on every entry of the second
 * vtable confirms that placement.
 *
 * The size is also pinned from outside the class, which is the stronger argument. `Rnd::TransAnim`
 * and `Rnd::View` each derive from this class at offset 0 and place `Rnd::Drawable` at offset 0x18,
 * so the next base begins exactly 0x18 bytes in. A constructor's highest store would only give a
 * lower bound.
 *
 * Two vtables belong to the class. The four-entry table at `0x0081e7f0` is addressed by the vptr
 * at `+0x14` and stores the three virtuals declared here. The nine-entry table at `0x0081e818` is
 * addressed by the `Rnd::Object` subobject vptr and stores the overrides of the `Rnd::Object`
 * virtuals, each with a `-0x18` adjustment back to the Animatable subobject. Slot 5 of the second
 * table still addresses the pure-virtual stub at `0x005381a8`. ClassName() is therefore
 * unimplemented here and the class remains abstract.
 *
 * An animatable owns two lists. mAnims stores the animatables this one drives, and every entry
 * registers this object as a referrer through Rnd::Object::AddRef(). mFilters stores a chain of
 * value filters this object owns outright and deletes in its destructor. SetFrame() maps the
 * incoming frame forward through that chain before handing it to the SetFrameSelf() hook and to
 * the children, and InverseFilters() maps a child frame back the other way.
 */
class Animatable : public virtual Object {
    // CollectChildren() in rnd/collectchildren.h walks mAnims directly, and the image has no
    // accessor for it.
    friend void CollectChildren(std::list<Object *> &objects, Animatable *pAnimatable);
    // MetRenderer::AddScreenView() and AddBackgroundView() search mAnims directly before
    // AddAnim(), and the image has no accessor for it.
    friend class ::MetRenderer;

public:
    /**
     * Value a filter reports from Filter::Type() and a `.rnd` file stores in front of its payload.
     *
     * The five names come from the type-name dumper at `0x0049a8a8`, whose switch table pairs each
     * tag with its literal. Nothing invokes that routine in the shipped build. The literals are
     * therefore the only surviving record of the enumeration.
     */
    enum FilterType {
        kFilterScaleOffset = 0, /*!< A ScaleOffset filter. */
        kFilterMinMaxLoop = 1,  /*!< A MinMaxLoop filter. */
        kFilterZeroOrder = 2,   /*!< A ZeroOrder filter. */
        kFilterFirstOrder = 3,  /*!< A FirstOrder filter. */
        kFilterSecondOrder = 4  /*!< A SecondOrder filter. */
    };

    /**
     * One stage of the frame-number filter chain.
     *
     * Its RTTI descriptor is at `0x0086f710`. It is a leaf class with no base. No accessor of its
     * own builds that descriptor. Every derived accessor initialises it inline before its own, and
     * the copy at `0x0040f540` builds it alone.
     *
     * The class declares no data member. A derived filter therefore stores its vptr at `+0x00` and
     * its parameters from `+0x04`. Every vtable in the family is eight entries, and the declaration
     * order below reproduces that order. The class emits no vtable of its own and appears in none
     * of the five tables. That absence is consistent with an abstract class the program never
     * instantiates.
     *
     * The class declares no destructor, virtual or otherwise. `~Animatable()` destroys a filter
     * through a bare `operator delete` with no vtable dispatch at all. A trivially destructible
     * filter compiles to exactly that.
     */
    class Filter {
    public:
        /**
         * Map a value forward through this stage.
         *
         * Vtable slot 1. Pure. All five subclasses supply a body.
         *
         * @param flValue The value to map.
         * @return The mapped value.
         */
        virtual float Apply(float flValue) = 0;

        /**
         * Map a value back through this stage.
         *
         * Vtable slot 2. The base implementation returns the value unchanged, and MinMaxLoop is
         * the one subclass that inherits it. The alternative reading, that `0x0049a560` is
         * MinMaxLoop's own override and this slot is pure as well, produces identical behaviour
         * and cannot be ruled out from the image.
         *
         * @param flValue The value to map.
         * @return The mapped value.
         * @ghidraAddress NTSC-U/C: 0x0049a560
         * @ghidraAddress PAL: 0x004d84c8
         */
        virtual float Inverse(float flValue);

        /**
         * Write this stage's parameters to sink.
         *
         * Vtable slot 3. Pure. FailSink::Print() discards its text in the shipped build. No
         * subclass produces output on this target.
         *
         * @param sink The diagnostic sink to write to.
         */
        virtual void Dump(FailSink &sink) = 0;

        /**
         * Write this stage's parameters to stream.
         *
         * Vtable slot 4. Pure. The type tag is written by the list writer rather than here.
         *
         * @param stream The stream to write to.
         */
        virtual void Save(Stream &stream) = 0;

        /**
         * Read this stage's parameters from stream.
         *
         * Vtable slot 5. Pure.
         *
         * @param stream The stream to read from.
         */
        virtual void Load(Stream &stream) = 0;

        /**
         * Report the type tag of this stage.
         *
         * Vtable slot 6. Pure. The returned value is one of the FilterType constants.
         *
         * @return The type tag.
         */
        virtual int Type() = 0;

        /**
         * Replace this stage's parameters with those of pSource.
         *
         * Vtable slot 7. Pure. Every subclass body is a block copy of the whole object, the vtable
         * pointer included. The argument must therefore already be of this stage's own type. Only
         * Animatable::Copy() invokes it, and it pairs the call with NewFilter() on the source tag.
         *
         * @param pSource The filter to copy the parameters of.
         */
        virtual void Copy(const Filter *pSource) = 0;
    };

    /**
     * Filter stage that applies an affine map.
     *
     * Its RTTI descriptor is at `0x008ef018`. It derives publicly from Filter at offset 0. The
     * object is 0xc bytes, and its vtable is at `0x0081e980`.
     */
    class ScaleOffset : public Filter {
    public:
        /**
         * Construct a stage with indeterminate parameters.
         *
         * NewFilter() builds a ScaleOffset this way and writes nothing beyond the vtable pointer,
         * so the two parameters start indeterminate and the reader overwrites both.
         */
        ScaleOffset() {
        }

        /**
         * Construct a stage with both parameters set.
         *
         * @param flScale The multiplier.
         * @param flOffset The addend.
         */
        ScaleOffset(float flScale, float flOffset) : mScale(flScale), mOffset(flOffset) {
        }

        /**
         * Scale and then offset the value.
         *
         * @param flValue The value to map.
         * @return flValue times mScale plus mOffset.
         * @ghidraAddress NTSC-U/C: 0x00498ef0
         * @ghidraAddress PAL: 0x004d6e50
         */
        virtual float Apply(float flValue);

        /**
         * Undo the affine map.
         *
         * A zero mScale divides by zero rather than reporting. That matches the binary.
         *
         * @param flValue The value to map.
         * @return flValue less mOffset, divided by mScale.
         * @ghidraAddress NTSC-U/C: 0x00498f08
         * @ghidraAddress PAL: 0x004d6e68
         */
        virtual float Inverse(float flValue);

        /**
         * Write "(scale:%.2f offset:%.2f)" to sink.
         *
         * @param sink The diagnostic sink to write to.
         * @ghidraAddress NTSC-U/C: 0x00498f20
         * @ghidraAddress PAL: 0x004d6e80
         */
        virtual void Dump(FailSink &sink);

        /**
         * Write both parameters to stream.
         *
         * @param stream The stream to write to.
         * @ghidraAddress NTSC-U/C: 0x00498fc0
         * @ghidraAddress PAL: 0x004d6f20
         */
        virtual void Save(Stream &stream);

        /**
         * Read both parameters from stream.
         *
         * @param stream The stream to read from.
         * @ghidraAddress NTSC-U/C: 0x00499030
         * @ghidraAddress PAL: 0x004d6f90
         */
        virtual void Load(Stream &stream);

        /**
         * Report kFilterScaleOffset.
         *
         * @return The type tag.
         * @ghidraAddress NTSC-U/C: 0x00499090
         * @ghidraAddress PAL: 0x004d6ff0
         */
        virtual int Type();

        /**
         * Copy both parameters from pSource.
         *
         * @param pSource The ScaleOffset to copy.
         * @ghidraAddress NTSC-U/C: 0x00499098
         * @ghidraAddress PAL: 0x004d6ff8
         */
        virtual void Copy(const Filter *pSource);

        // Public because Animatable::SetRate() and Animatable::SetOffset() write both parameters
        // directly, and the image has no accessor to route those writes through.

        float mScale;  /*!< The multiplier. */
        float mOffset; /*!< The addend. */
    };

    /**
     * Filter stage that wraps or clamps the value into a range.
     *
     * Its RTTI descriptor is at `0x008eee98`. It derives publicly from Filter at offset 0. The
     * object is 0x10 bytes, and its vtable is at `0x0081e938`.
     */
    class MinMaxLoop : public Filter {
    public:
        /**
         * Construct a stage with indeterminate parameters.
         *
         * @ghidraAddress NTSC-U/C: 0x004990b8
         * @ghidraAddress PAL: 0x004d7018
         */
        MinMaxLoop() {
        }

        /**
         * Construct a stage with all three parameters set.
         *
         * @param flMin The lower end of the range.
         * @param flMax The upper end of the range.
         * @param nLoop Non-zero to wrap into the range instead of clamping to it.
         * @ghidraAddress NTSC-U/C: 0x004990d0
         * @ghidraAddress PAL: 0x004d7030
         */
        MinMaxLoop(float flMin, float flMax, int nLoop) : mMin(flMin), mMax(flMax), mLoop(nLoop) {
        }

        /**
         * Bring the value into the range.
         *
         * A set mLoop wraps the value around the range, and a clear mLoop clamps it to the ends.
         * The wrapping branch divides by zero when mMin equals mMax.
         *
         * @param flValue The value to map.
         * @return The value brought into the range.
         * @ghidraAddress NTSC-U/C: 0x0049a4b8
         * @ghidraAddress PAL: 0x004d8420
         */
        virtual float Apply(float flValue);

        /**
         * Write "(min:%.2f max:%.2f loop:true)" to sink, with mLoop as a word rather than a number.
         *
         * @param sink The diagnostic sink to write to.
         * @ghidraAddress NTSC-U/C: 0x004990f0
         * @ghidraAddress PAL: 0x004d7050
         */
        virtual void Dump(FailSink &sink);

        /**
         * Write the two ends as floats and mLoop as a single byte.
         *
         * @param stream The stream to write to.
         * @ghidraAddress NTSC-U/C: 0x004991c8
         * @ghidraAddress PAL: 0x004d7128
         */
        virtual void Save(Stream &stream);

        /**
         * Read the two ends as floats and mLoop as a single byte.
         *
         * @param stream The stream to read from.
         * @ghidraAddress NTSC-U/C: 0x00499258
         * @ghidraAddress PAL: 0x004d71b8
         */
        virtual void Load(Stream &stream);

        /**
         * Report kFilterMinMaxLoop.
         *
         * @return The type tag.
         * @ghidraAddress NTSC-U/C: 0x004992e0
         * @ghidraAddress PAL: 0x004d7240
         */
        virtual int Type();

        /**
         * Copy all three parameters from pSource.
         *
         * @param pSource The MinMaxLoop to copy.
         * @ghidraAddress NTSC-U/C: 0x004992e8
         * @ghidraAddress PAL: 0x004d7248
         */
        virtual void Copy(const Filter *pSource);

        // Public because Animatable::SetLoopRange() writes both ends directly, and the image has
        // no accessor to route those writes through.

        float mMin; /*!< The lower end of the range. */
        float mMax; /*!< The upper end of the range. */

    private:
        int mLoop; // +0x0c
    };

    /**
     * Filter stage that limits how far the value may move in one frame.
     *
     * Its RTTI descriptor is at `0x00902040`. It derives publicly from Filter at offset 0. The
     * object is 0xc bytes, and its vtable is at `0x0081e8f0`.
     *
     * The stage is stateful. mLevel is both a parameter and the running output, and each Apply()
     * advances it towards the incoming value by at most mMaxDelta.
     */
    class ZeroOrder : public Filter {
    public:
        /** Construct a stage with indeterminate parameters. */
        ZeroOrder() {
        }

        /**
         * Construct a stage with both parameters set.
         *
         * @param flLevel The starting output.
         * @param flMaxDelta The largest step one Apply() may take.
         */
        ZeroOrder(float flLevel, float flMaxDelta) : mLevel(flLevel), mMaxDelta(flMaxDelta) {
        }

        /**
         * Advance mLevel towards the value by at most mMaxDelta.
         *
         * @param flValue The value to move towards.
         * @return The new mLevel.
         * @ghidraAddress NTSC-U/C: 0x0049a568
         * @ghidraAddress PAL: 0x004d84d0
         */
        virtual float Apply(float flValue);

        /**
         * Report the mLevel that preceded an Apply() of the value.
         *
         * A value below mLevel means the last step moved down. The earlier level was therefore one
         * step higher, and the reverse for a value above. An equal value reports mLevel unchanged.
         *
         * @param flValue The value the last Apply() received.
         * @return The earlier level.
         * @ghidraAddress NTSC-U/C: 0x0049a5b8
         * @ghidraAddress PAL: 0x004d8520
         */
        virtual float Inverse(float flValue);

        /**
         * Write "(level:%.2f maxDelta:%.2f)" to sink.
         *
         * @param sink The diagnostic sink to write to.
         * @ghidraAddress NTSC-U/C: 0x00499348
         * @ghidraAddress PAL: 0x004d72a8
         */
        virtual void Dump(FailSink &sink);

        /**
         * Write both parameters to stream.
         *
         * @param stream The stream to write to.
         * @ghidraAddress NTSC-U/C: 0x004993e8
         * @ghidraAddress PAL: 0x004d7348
         */
        virtual void Save(Stream &stream);

        /**
         * Read both parameters from stream.
         *
         * @param stream The stream to read from.
         * @ghidraAddress NTSC-U/C: 0x00499458
         * @ghidraAddress PAL: 0x004d73b8
         */
        virtual void Load(Stream &stream);

        /**
         * Report kFilterZeroOrder.
         *
         * @return The type tag.
         * @ghidraAddress NTSC-U/C: 0x004994b8
         * @ghidraAddress PAL: 0x004d7418
         */
        virtual int Type();

        /**
         * Copy both parameters from pSource.
         *
         * @param pSource The ZeroOrder to copy.
         * @ghidraAddress NTSC-U/C: 0x004994c0
         * @ghidraAddress PAL: 0x004d7420
         */
        virtual void Copy(const Filter *pSource);

    private:
        float mLevel;    // +0x04
        float mMaxDelta; // +0x08
    };

    /**
     * Filter stage that moves the value a fixed fraction of the way towards its input.
     *
     * Its RTTI descriptor is at `0x008eef08`. It derives publicly from Filter at offset 0. The
     * object is 0xc bytes, and its vtable is at `0x0081e8a8`.
     *
     * The stage is stateful in the same way as ZeroOrder. mLevel is the running output.
     */
    class FirstOrder : public Filter {
    public:
        /** Construct a stage with indeterminate parameters. */
        FirstOrder() {
        }

        /**
         * Construct a stage with both parameters set.
         *
         * @param flLevel The starting output.
         * @param flRatio The fraction of the remaining distance each Apply() covers.
         */
        FirstOrder(float flLevel, float flRatio) : mLevel(flLevel), mRatio(flRatio) {
        }

        /**
         * Move mLevel a fraction mRatio of the way towards the value.
         *
         * @param flValue The value to move towards.
         * @return The new mLevel.
         * @ghidraAddress NTSC-U/C: 0x004994e0
         * @ghidraAddress PAL: 0x004d7440
         */
        virtual float Apply(float flValue);

        /**
         * Report the mLevel that preceded an Apply() of the value.
         *
         * A unit mRatio divides by zero rather than reporting. That matches the binary.
         *
         * @param flValue The value the last Apply() received.
         * @return The earlier level.
         * @ghidraAddress NTSC-U/C: 0x00499500
         * @ghidraAddress PAL: 0x004d7460
         */
        virtual float Inverse(float flValue);

        /**
         * Write "(level:%.2f ratio:%.2f)" to sink.
         *
         * @param sink The diagnostic sink to write to.
         * @ghidraAddress NTSC-U/C: 0x00499528
         * @ghidraAddress PAL: 0x004d7488
         */
        virtual void Dump(FailSink &sink);

        /**
         * Write both parameters to stream.
         *
         * @param stream The stream to write to.
         * @ghidraAddress NTSC-U/C: 0x004995c8
         * @ghidraAddress PAL: 0x004d7528
         */
        virtual void Save(Stream &stream);

        /**
         * Read both parameters from stream.
         *
         * @param stream The stream to read from.
         * @ghidraAddress NTSC-U/C: 0x00499638
         * @ghidraAddress PAL: 0x004d7598
         */
        virtual void Load(Stream &stream);

        /**
         * Report kFilterFirstOrder.
         *
         * @return The type tag.
         * @ghidraAddress NTSC-U/C: 0x00499698
         * @ghidraAddress PAL: 0x004d75f8
         */
        virtual int Type();

        /**
         * Copy both parameters from pSource.
         *
         * @param pSource The FirstOrder to copy.
         * @ghidraAddress NTSC-U/C: 0x004996a0
         * @ghidraAddress PAL: 0x004d7600
         */
        virtual void Copy(const Filter *pSource);

    private:
        float mLevel; // +0x04
        float mRatio; // +0x08
    };

    /**
     * Filter stage that drives the value with a spring and a damper.
     *
     * Its RTTI descriptor is at `0x008eefa8`. It derives publicly from Filter at offset 0. The
     * object is 0x14 bytes, and its vtable is at `0x0081e860`.
     *
     * The stage is stateful in two members. mLevel is the running output and mVel the running
     * velocity. Each Apply() accelerates the velocity towards the incoming value and then
     * integrates it once.
     */
    class SecondOrder : public Filter {
    public:
        /**
         * Construct a stage at rest with indeterminate spring parameters.
         *
         * NewFilter() clears mVel and writes nothing else beyond the vtable pointer.
         */
        SecondOrder() : mVel(0.0f) {
        }

        /**
         * Construct a stage at rest with all three parameters set.
         *
         * @param flLevel The starting output.
         * @param flSpring The acceleration per unit of distance to the input.
         * @param flDamper The fraction of the velocity shed each Apply().
         */
        SecondOrder(float flLevel, float flSpring, float flDamper)
            : mLevel(flLevel), mSpring(flSpring), mDamper(flDamper), mVel(0.0f) {
        }

        /**
         * Integrate the spring and the damper by one step.
         *
         * @param flValue The value to move towards.
         * @return The new mLevel.
         * @ghidraAddress NTSC-U/C: 0x0049a5f8
         * @ghidraAddress PAL: 0x004d8560
         */
        virtual float Apply(float flValue);

        /**
         * Report the mLevel that preceded the last Apply().
         *
         * The argument is ignored. One step back is the current level less the current velocity.
         *
         * @param flValue The value the last Apply() received.
         * @return The earlier level.
         * @ghidraAddress NTSC-U/C: 0x0049a630
         * @ghidraAddress PAL: 0x004d8598
         */
        virtual float Inverse(float flValue);

        /**
         * Write "(level:%.2f spring:%.2f damper:)%.2f vel%.2f" to sink.
         *
         * The closing bracket lands one call too early in the binary, in front of the damper value
         * instead of after the velocity, and the " vel" label has no colon. Both are faults of the
         * original text rather than of the recovery.
         *
         * @param sink The diagnostic sink to write to.
         * @ghidraAddress NTSC-U/C: 0x00499700
         * @ghidraAddress PAL: 0x004d7660
         */
        virtual void Dump(FailSink &sink);

        /**
         * Write the three spring parameters to stream, the velocity excluded.
         *
         * @param stream The stream to write to.
         * @ghidraAddress NTSC-U/C: 0x00499800
         * @ghidraAddress PAL: 0x004d7760
         */
        virtual void Save(Stream &stream);

        /**
         * Read the three spring parameters from stream, the velocity excluded.
         *
         * A stage read this way therefore retains whatever velocity it already had, and one built
         * by NewFilter() starts at rest.
         *
         * @param stream The stream to read from.
         * @ghidraAddress NTSC-U/C: 0x00499890
         * @ghidraAddress PAL: 0x004d77f0
         */
        virtual void Load(Stream &stream);

        /**
         * Report kFilterSecondOrder.
         *
         * @return The type tag.
         * @ghidraAddress NTSC-U/C: 0x00499908
         * @ghidraAddress PAL: 0x004d7868
         */
        virtual int Type();

        /**
         * Copy all four members from pSource.
         *
         * @param pSource The SecondOrder to copy.
         * @ghidraAddress NTSC-U/C: 0x00499910
         * @ghidraAddress PAL: 0x004d7870
         */
        virtual void Copy(const Filter *pSource);

    private:
        float mLevel;  // +0x04
        float mSpring; // +0x08
        float mDamper; // +0x0c
        float mVel;    // +0x10
    };

    /**
     * Construct an animatable with no children, no filters, and a zero frame.
     *
     * @ghidraAddress NTSC-U/C: 0x0049a108
     * @ghidraAddress PAL: 0x004d8068
     */
    Animatable();

    /**
     * Drop this object's references on its children and delete its filters.
     *
     * @ghidraAddress NTSC-U/C: 0x00499f48
     * @ghidraAddress PAL: 0x004d7ea8
     */
    virtual ~Animatable();

    /**
     * Report the animatable that animates this one.
     *
     * Walks the referrer list of the `Rnd::Object` subobject, casts each referrer to Animatable and
     * returns the first whose own mAnims list includes this object. The result is therefore the
     * parent in the animation hierarchy rather than a plain cast.
     *
     * @return The parent animatable, or null when no referrer animates this one.
     * @ghidraAddress NTSC-U/C: 0x00494a88
     * @ghidraAddress PAL: 0x004d2950
     */
    Animatable *Parent();

    /**
     * Append pAnim to mAnims.
     *
     * Registers this object as a referrer of pAnim. A pAnim already in mAnims produces the report
     * "%s already in %s" and no insertion.
     *
     * @param pAnim The animatable to add.
     * @ghidraAddress NTSC-U/C: 0x004953c0
     * @ghidraAddress PAL: 0x004d3288
     */
    void AddAnim(Animatable *pAnim);

    /**
     * Erase pAnim from mAnims.
     *
     * Drops this object's reference on pAnim first. A pAnim absent from mAnims does nothing.
     *
     * @param pAnim The animatable to remove.
     * @ghidraAddress NTSC-U/C: 0x00495540
     * @ghidraAddress PAL: 0x004d3408
     */
    void RemoveAnim(Animatable *pAnim);

    /**
     * Drive this object and its whole subtree to a frame.
     *
     * Records the frame as given, maps it forward through mFilters, records the filtered result,
     * hands the filtered result to the SetFrameSelf() hook, and then passes the same filtered
     * result to every mAnims entry. A child therefore receives its parent's filtered frame and
     * applies its own filters to it in turn.
     *
     * @param flFrame The frame to move to.
     * @ghidraAddress NTSC-U/C: 0x0049a428
     * @ghidraAddress PAL: 0x004d8390
     */
    void SetFrame(float flFrame);

    /**
     * Map a value back through mFilters.
     *
     * Walks mFilters from back to front and applies Filter::Inverse() at each stage. That undoes
     * what SetFrame() does on the way in. EndFrame() uses it to bring a child's end frame into
     * this object's frame numbering.
     *
     * @param flValue The value to map.
     * @return The value in this object's incoming frame numbering.
     * @ghidraAddress NTSC-U/C: 0x00495050
     * @ghidraAddress PAL: 0x004d2f18
     */
    float InverseFilters(float flValue);

    /**
     * Append an already-built filter to mFilters.
     *
     * The list takes ownership, and the destructor deletes the filter.
     *
     * @param pFilter The filter to append.
     * @ghidraAddress NTSC-U/C: 0x00499940
     * @ghidraAddress PAL: 0x004d78a0
     */
    void AddFilter(Filter *pFilter);

    /**
     * Append a new ScaleOffset stage to mFilters.
     *
     * @param flScale The multiplier.
     * @param flOffset The addend.
     * @ghidraAddress NTSC-U/C: 0x004999e8
     * @ghidraAddress PAL: 0x004d7948
     */
    void AddScaleOffset(float flScale, float flOffset);

    /**
     * Append a new MinMaxLoop stage to mFilters.
     *
     * @param flMin The lower end of the range.
     * @param flMax The upper end of the range.
     * @param nLoop Non-zero to wrap into the range instead of clamping to it.
     * @ghidraAddress NTSC-U/C: 0x00499ae0
     * @ghidraAddress PAL: 0x004d7a40
     */
    void AddMinMaxLoop(float flMin, float flMax, int nLoop);

    /**
     * Append a new ZeroOrder stage to mFilters.
     *
     * @param flLevel The starting output.
     * @param flMaxDelta The largest step one frame may take.
     * @ghidraAddress NTSC-U/C: 0x00499be8
     * @ghidraAddress PAL: 0x004d7b48
     */
    void AddZeroOrder(float flLevel, float flMaxDelta);

    /**
     * Append a new FirstOrder stage to mFilters.
     *
     * @param flLevel The starting output.
     * @param flRatio The fraction of the remaining distance each frame covers.
     * @ghidraAddress NTSC-U/C: 0x00499ce0
     * @ghidraAddress PAL: 0x004d7c40
     */
    void AddFirstOrder(float flLevel, float flRatio);

    /**
     * Append a new SecondOrder stage to mFilters.
     *
     * @param flLevel The starting output.
     * @param flSpring The acceleration per unit of distance to the input.
     * @param flDamper The fraction of the velocity shed each frame.
     * @ghidraAddress NTSC-U/C: 0x00499dd8
     * @ghidraAddress PAL: 0x004d7d38
     */
    void AddSecondOrder(float flLevel, float flSpring, float flDamper);

    /**
     * Delete and erase the filter at one position in mFilters.
     *
     * An index at or past the end does nothing. No call site survives in the shipped build.
     *
     * @param nIndex How far into mFilters the filter sits.
     * @ghidraAddress NTSC-U/C: 0x00494c00
     * @ghidraAddress PAL: 0x004d2ac8
     */
    void RemoveFilter(int nIndex);

    /**
     * Return the filter at one position in mFilters.
     *
     * The position is not tested against the list. The out-of-line copy has no caller, and the
     * title is inferred.
     *
     * @param nIndex How far into mFilters the filter sits.
     * @return The filter.
     * @ghidraAddress NTSC-U/C: 0x0049a0b0
     * @ghidraAddress PAL: 0x004d8010
     */
    Filter *FilterAt(int nIndex);

    /**
     * Change the multiplier of the ScaleOffset stage at the front of mFilters.
     *
     * The addend is recomputed as well, so the stage maps the current mFrame to the same output
     * as before and only later frames advance at the new rate. An empty chain, or a front stage of
     * another type, produces the fatal report "%s must have scale-offset filter in slot 1". The
     * definition sits in the unit of the tunnel object cache. The title is inferred.
     *
     * @param flRate The new multiplier.
     * @ghidraAddress NTSC-U/C: 0x0040d2b0
     * @ghidraAddress PAL: 0x00446d10
     */
    void SetRate(float flRate);

    /**
     * Change the addend of the ScaleOffset stage at the front of mFilters.
     *
     * An empty chain, or a front stage of another type, produces the fatal report "%s must have
     * scale-offset filter in slot 1". The definition sits in the unit of the tunnel object cache.
     * The title is inferred.
     *
     * @param flOffset The new addend.
     * @ghidraAddress NTSC-U/C: 0x0040d3d0
     * @ghidraAddress PAL: 0x00446e30
     */
    void SetOffset(float flOffset);

    /**
     * Change both ends of the MinMaxLoop stage at the back of mFilters.
     *
     * An empty chain, or a back stage of another type, produces the fatal report "%s must have
     * min-max-loop filter in last slot". The definition sits in the unit of the tunnel object
     * cache. The title is inferred.
     *
     * @param flMin The new lower end.
     * @param flMax The new upper end.
     * @ghidraAddress NTSC-U/C: 0x0040d4a8
     * @ghidraAddress PAL: 0x00446f08
     */
    void SetLoopRange(float flMin, float flMax);

    /**
     * Build one default-constructed filter for a type tag.
     *
     * The reader and Copy() both use this to turn a stored tag back into an object. A tag outside
     * the FilterType range produces the report path of g_failSink and a null result.
     *
     * @param nType One of the FilterType constants.
     * @return The new filter, or null for an unrecognised tag.
     * @ghidraAddress NTSC-U/C: 0x004950e8
     * @ghidraAddress PAL: 0x004d2fb0
     */
    static Filter *NewFilter(int nType);

    /**
     * Report the last frame this object's subtree animates to.
     *
     * Animatable vtable slot 1. The base implementation animates nothing of its own and reports the
     * largest child end frame, each brought into this object's frame numbering through that child's
     * own filter chain, starting from zero. Rnd::MeshAnim, Rnd::MatAnim, and Rnd::LightAnim
     * override it with the timestamp of their last keyframe.
     *
     * @return The last frame, never below zero.
     * @ghidraAddress NTSC-U/C: 0x00494b50
     * @ghidraAddress PAL: 0x004d2a18
     */
    virtual float EndFrame();

    /**
     * Restart the animation of this object and its whole subtree.
     *
     * Animatable vtable slot 2. The base implementation forwards to the same slot on every mAnims
     * entry. Rnd::ParticleSys overrides it at `0x0052c490`, releases its live particles, and then
     * chains to this implementation. The title is inferred from that one override. No string in the
     * image identifies the slot, and no direct call site survives.
     *
     * @ghidraAddress NTSC-U/C: 0x0049a3b8
     * @ghidraAddress PAL: 0x004d8320
     */
    virtual void StartAnim();

    /**
     * Write a description of this object to sink.
     *
     * Writes mFilters and mAnims, and produces nothing at all when the dump level of sink is not
     * positive. FailSink::Print() discards its text in the shipped build. The routine therefore
     * produces no output on this target in any case.
     *
     * @param sink The diagnostic sink to write to.
     * @ghidraAddress NTSC-U/C: 0x0049a640
     * @ghidraAddress PAL: 0x004d85a8
     */
    virtual void DumpText(FailSink &sink);

    /**
     * Write the revision, mFilters, and mAnims to stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x0049a6e8
     * @ghidraAddress PAL: 0x004d8650
     */
    virtual void Save(Stream &stream);

    /**
     * Retarget every mAnims entry equal to pFrom at pTo.
     *
     * @param pFrom The object being replaced.
     * @param pTo The replacement, or null.
     * @ghidraAddress NTSC-U/C: 0x004951e0
     * @ghidraAddress PAL: 0x004d30a8
     */
    virtual void Replace(Object *pFrom, Object *pTo);

    /**
     * Clone the filter chain of pSource, and its child list when nFlags requests it.
     *
     * @param pSource The object to copy from.
     * @param nFlags The set of fields to copy; see kCopyChildLists.
     * @ghidraAddress NTSC-U/C: 0x00494e70
     * @ghidraAddress PAL: 0x004d2d38
     */
    virtual void Copy(const Object *pSource, unsigned nFlags);

    /**
     * Replace the filter chain and the child list from stream.
     *
     * A revision above the one this build writes produces the report "Can't load new Animatable"
     * followed by the abort handler of g_failSink.
     *
     * @param stream The stream to read from.
     * @ghidraAddress NTSC-U/C: 0x00494d68
     * @ghidraAddress PAL: 0x004d2c30
     */
    virtual void Load(Stream &stream);

    /**
     * Drop this object's reference on every mAnims entry and then empty mAnims.
     *
     * Every derived destructor invokes this before its own teardown. The tail at `0x0049a9c0`
     * passes the address of mAnims to the list clear at `0x0045edc8`, whose body returns every
     * node to the pool and re-self-links the dummy node. ReleaseAnimsAndFilters() at `0x00494cb0`
     * is the routine that walks mAnims without emptying it, and the two bodies are otherwise alike.
     *
     * Public because Rnd::MetScreen::ResolveContainerViews() calls it at `0x0038b294` on the
     * Rnd::View it just resolved, and MetScreen derives from MsgSink rather than from this class.
     * A friend declaration for MetScreen fits the image equally well.
     *
     * @ghidraAddress NTSC-U/C: 0x0049a960
     * @ghidraAddress PAL: 0x004d88c8
     */
    void ReleaseAnimsRefs();

protected:
    /**
     * Animate this object alone to a frame.
     *
     * Animatable vtable slot 3. The base implementation animates nothing. Every animation subclass
     * overrides it to interpolate its keyframes at the frame SetFrame() has already filtered. Only
     * SetFrame() invokes it. The arrangement matches Rnd::Drawable::DrawSelf().
     *
     * @param flFrame The filtered frame to animate to.
     * @ghidraAddress NTSC-U/C: 0x0049a100
     * @ghidraAddress PAL: 0x004d8060
     */
    virtual void SetFrameSelf(float flFrame);

private:
    // NTSC-U/C: 0x00494cb0, PAL: 0x004d2b78
    // Drops this object's reference on every mAnims entry, deletes every filter, and
    // then empties mFilters. The destructor, Copy(), and Load() all invoke it. That shared use is
    // what makes it a member rather than the destructor body alone. The title is inferred.
    void ReleaseAnimsAndFilters();
    // NTSC-U/C: 0x0049a7c0, PAL: 0x004d8728
    // Only SetFrame() invokes this.
    float ApplyFilters(float flValue);
    // NTSC-U/C: 0x0049a750, PAL: 0x004d86b8
    // Only Copy() and Load() invoke this, and both inline it.
    void AcquireAnimsRefs();

    // Declared in recovered offset order. The animatables this one drives, each of which
    // registers this object as a referrer.
    std::list<Animatable *> mAnims; // +0x04
    // The filter chain, owned outright and deleted by the destructor.
    std::list<Filter *> mFilters; // +0x08
    // The frame SetFrame() last received, before the filter chain.
    float mFrame; // +0x0c

public:
    /**
     * The frame SetFrame() last received, after the filter chain.
     *
     * This is the value handed to SetFrameSelf() and to every child. Public because
     * TnlCrippleFX::SetFrame() at `0x0043e500` reads it through the crippler path, and the image
     * has no accessor. +0x10
     */
    float mFilteredFrame;
};

} // namespace Rnd
