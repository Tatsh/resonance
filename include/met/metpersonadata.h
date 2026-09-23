#pragma once

#include <cstddef>
#include <vector>

#include "game/campaignstats.h"
#include "game/freqappearance.h"
#include "os/hxstr.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

/**
 * Saved record of one player persona.
 *
 * `14MetPersonaData` in the RTTI descriptor at `0x0086f610`, a leaf class with no base. Following
 * the g++ 2.x layout for a class with no base, the vptr sits after the data members, here at
 * `+0x168`, so the object is 0x16c bytes. The four-entry vtable is at `0x00804bd8`.
 *
 * The class supplies both its own allocation and its own release function, and both tag the block
 * with the literal `MetPersonaData` at `0x00804a78`.
 *
 * Two of its members are objects with virtuals of their own, and an earlier reading recorded both
 * as unidentified reserved spans. Both are now settled on four matching facts each. The member at
 * offset 0 is a CampaignStats, 0x140 bytes with its vptr at `+0x13c`, built by
 * CampaignStats::CampaignStats() at `0x00140580` and torn down by its destructor at `0x00140768`.
 * The member at `+0x140` is a FreqAppearance, 0x14 bytes with its vptr at `+0x10`, built at
 * `0x001745b8` and torn down by the destructor its vtable slot 1 records at `0x00174730`.
 *
 * Vtable slots 2 and 3, at `0x0032b880` and `0x0032b968`, are a matched pair. Slot 2 hands the
 * constant 4-byte value 2 to its argument through the argument's vtable slot 4 and then forwards
 * the same argument to slot 2 of both member objects. Slot 3 takes a 4-byte value back through
 * slot 6 and forwards to slot 3 of both members. The pair is therefore the persist-and-restore
 * pair with a record version of 2.
 *
 * An earlier reading recorded the argument's class as unidentified. The two slot numbers settle it.
 * Slot 4 of OBStream is `Write(const void *, int)` and slot 6 of IBStream is `Read(void *, int)`,
 * and GameManagerImpl calls both of those slots on both of those classes from its own
 * persist-and-restore pair.
 */
class MetPersonaData {
public:
    /**
     * Allocate an instance from the tagged heap.
     *
     * @param nSize The object size, which the compiler supplies.
     * @return The block.
     * @ghidraAddress 0x0032e1e8
     */
    void *operator new(size_t nSize);

    /**
     * Release an instance to the tagged heap.
     *
     * @param pBlock The block.
     * @ghidraAddress 0x0032e208
     */
    void operator delete(void *pBlock);

    /**
     * Construct an empty record.
     *
     * @ghidraAddress 0x0032b760
     */
    MetPersonaData();

    /**
     * Resolve the list of records the memory-card path last read.
     *
     * The list is a function-local static vector at `0x00891ab8` behind the guard flag at
     * `0x00699d78`, in the accessor at `0x00215ca0`, which registers its destructor through
     * `atexit`. `0x00215cf8` fills it and eight front-end screens read it, among them
     * MetExpansionPakScreen, MetLoadFreqScreen, MetMemCardLoadScreen, MetPersonaSaverScreen, and
     * MetLocPickCharScreen. The address recorded here is the single out-of-line emission of the
     * inline accessor those eight share, and it forwards to the static holder.
     *
     * It is a static member rather than a free function, because it takes no receiver and vends
     * exactly one class. The owning class is an inference from the element type alone, since no
     * descriptor, literal, or file path in the image attributes either address.
     *
     * @return The list. It is never null.
     * @ghidraAddress 0x00218118
     */
    static std::vector<MetPersonaData *> *loadList();

    /**
     * @ghidraAddress 0x0032e278
     */
    virtual ~MetPersonaData();

    /**
     * Copy every member from another record.
     *
     * GameManagerImpl::AddPersona() is the one recovered caller. It default-constructs a heap
     * record and then runs this, rather than using a copy constructor, which is what establishes
     * the member as an assignment. The result is discarded there, so the return type is the
     * conventional one rather than a recovered one.
     *
     * @param other The record to copy.
     * @return This record.
     * @ghidraAddress 0x0032e420
     */
    MetPersonaData &operator=(const MetPersonaData &other);

    /**
     * Write the record to a stream.
     *
     * Slot 2. The record version 2 goes out first, then both member objects write themselves
     * through their own slot 2.
     *
     * The stream class was recorded as unidentified when this class first landed. It is OBStream,
     * which the slot the routine calls settles. Slot 4 of OBStream is `Write(const void *, int)`,
     * and GameManagerImpl::Save() calls the same slot on the same class.
     *
     * @param pStream The stream to write to.
     * @ghidraAddress 0x0032b880
     */
    virtual void Save(OBStream *pStream);

    /**
     * Read the record back from a stream.
     *
     * Slot 3. The counterpart of Save(), reaching slot 6 of IBStream, which is
     * `Read(void *, int)`.
     *
     * @param pStream The stream to read from.
     * @ghidraAddress 0x0032b968
     */
    virtual void Load(IBStream *pStream);

    /**
     * Record in the appearance the highest skill the campaign has reached.
     *
     * The status is 4 when stage 5 is complete on expert and the secret stage is unlocked, 3 when
     * only that stage is complete, 2 when stage 4 is complete on normal, 1 when stage 3 is complete
     * on easy, and 0 otherwise. MetStageFinishScreen slot 5 is the caller.
     *
     * @ghidraAddress 0x0032e308
     */
    void UpdateSkillStatus();

    /**
     * Hang this persona's avatar in one of the four persona burn slots.
     *
     * Forwards to FreqAppearance::AttachToBurnSlot() on the embedded appearance.
     * MetTutorialScreen::OnUnknownSlot36() is a caller.
     *
     * @param nSlot The burn slot, 0 through 3.
     * @ghidraAddress 0x0032e488
     */
    void AttachToBurnSlot(int nSlot);

    /**
     * Campaign progress of this persona. +0x00
     *
     * Public because MetStageFinishScreen slot 5 records and queries the finished stage through it
     * directly, and the image has no accessor. The member sits at offset 0, which is why callers
     * pass the persona where a CampaignStats is expected.
     */
    CampaignStats mStats;

    /**
     * Appearance the front end displays and edits.
     *
     * Public rather than private, because MetLoadFreqBaseScreen::UpdateNameLabel() reads the
     * username out of it directly and the image has no accessor to route that read through. A
     * friend declaration fits the image equally well. +0x140
     */
    FreqAppearance mUnknown140;
    HxStr mUnknown154; // +0x154
    int mUnknown15c;   // +0x15c
    HxStr mUnknown160; // +0x160
};
