#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "game/idable.h"

/**
 * One participant in a session, local or remote.
 *
 * `Player` in the RTTI descriptor at `0x009020c0`, over three bases: `IDable<Player>` at offset 0,
 * `MsgSink` at 8, and `MsgSource` at 12. Three classes derive from it, `LocalPlayer`, `NetPlayer`,
 * and `NullPlayer`.
 *
 * Three vtables belong to it, each walked to its all-zero terminator rather than counted from the
 * slot titles, which understate every one of them. The primary at `0x007d2170` has 21 entries, so
 * this class declares 19 virtuals of its own after slot 0 and the destructor. The `MsgSink` table
 * at `0x007d2148` has 4 and overrides only `HandleMessage`, and the `MsgSource` table at
 * `0x007d2120` has 4 and overrides nothing, leaving `AddSink` and `RemoveSink` as inherited.
 *
 * Almost all of the primary table is inert defaults: slot 4 returns -1, six slots return 0, two
 * return 1, one returns 0.0f, and four are empty. Only slots 2, 3, 11, 12, and 13 have a real
 * body, so this class is an interface with defaults and the three subclasses carry the behaviour.
 *
 * The base subobjects account for `+0x00` through `+0x1f`, which the destructor at `0x00132ae8`
 * confirms by restoring a vptr at `+0x04` for `IDable<Player>`, at `+0x08` for `MsgSink`, and at
 * `+0x1c` for `MsgSource`, whose own `mSinks` vector it tears down at `+0x10`. This class's own
 * members start at `+0x20`, and the only one the destructor releases is the pointer at `+0x28`.
 *
 * A slot whose verb is unrecovered keeps its table index as its title, because the index is part
 * of the layout. The comment records the behaviour recovered instead.
 */
class Player : public IDable<Player>, public MsgSink, public MsgSource {
public:
    /** @ghidraAddress 0x00132ae8 */
    virtual ~Player();

    /** @ghidraAddress 0x00132c20 */
    virtual void Slot2();

    /** @ghidraAddress 0x00132c60 */
    virtual void Slot3();

    /**
     * Slot 4. Returns -1.
     *
     * @ghidraAddress 0x00132c98
     */
    virtual int Slot4();

    /**
     * Slot 5. Returns zero.
     *
     * @ghidraAddress 0x00132ca0
     */
    virtual int Slot5();

    /**
     * Slot 6. Returns zero.
     *
     * @ghidraAddress 0x00132ca8
     */
    virtual int Slot6();

    /**
     * Slot 7. Empty.
     *
     * @ghidraAddress 0x00132cb0
     */
    virtual void Slot7();

    /**
     * Slot 8. Empty.
     *
     * @ghidraAddress 0x00132cb8
     */
    virtual void Slot8();

    /**
     * Slot 9. Returns zero.
     *
     * @ghidraAddress 0x00132cc0
     */
    virtual int Slot9();

    /**
     * Slot 10. Returns zero.
     *
     * @ghidraAddress 0x00132cc8
     */
    virtual int Slot10();

    /** @ghidraAddress 0x0012f788 */
    virtual void Slot11();

    /** @ghidraAddress 0x00132d30 */
    virtual void Slot12();

    /** @ghidraAddress 0x00133110 */
    virtual void Slot13();

    /**
     * Slot 14. Empty.
     *
     * @ghidraAddress 0x00132d70
     */
    virtual void Slot14();

    /**
     * Slot 15. Empty.
     *
     * @ghidraAddress 0x00132d78
     */
    virtual void Slot15();

    /**
     * Slot 16. Returns 1.
     *
     * @ghidraAddress 0x00132d80
     */
    virtual int Slot16();

    /**
     * Slot 17. Returns zero.
     *
     * @ghidraAddress 0x00132d88
     */
    virtual int Slot17();

    /**
     * Slot 18. Returns 0.0f, which is what fixes the return type.
     *
     * @ghidraAddress 0x00132d90
     */
    virtual float Slot18();

    /**
     * Slot 19. Returns zero.
     *
     * @ghidraAddress 0x00132da0
     */
    virtual int Slot19();

    /**
     * Slot 20. Returns 1.
     *
     * @ghidraAddress 0x00132db0
     */
    virtual int Slot20();

    /**
     * Receive one message.
     *
     * The only `MsgSink` virtual this class overrides, at slot 3 of its `MsgSink` table.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x00133240
     */
    virtual void HandleMessage(Message *pMsg);

protected:
    // Declared in recovered offset order. The base subobjects occupy +0x00 through +0x1f.
    int mUnknown20; // +0x20
    int mUnknown24; // +0x24
    // Released by ~Player, so this member owns its allocation.
    void *mUnknown28; // +0x28
};
