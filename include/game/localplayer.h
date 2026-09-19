#pragma once

#include "app/msgsink.h"
#include "game/player.h"

/**
 * Player driven by a controller on this machine.
 *
 * `LocalPlayer` in the RTTI descriptor at `0x008eeff8`, with `Player` as its only base. Its three
 * vtables are at `0x007d0160`, `0x007d0138`, and `0x007d0110`, each walked to its terminator.
 *
 * The primary table has **23 entries against the base's 21**, so this class adds two virtuals past
 * the end of the base table rather than overriding into it. It replaces slots 2 and 4 through 12
 * and 14 through 20, and inherits only slots 3 and 13. In the `MsgSource` table it replaces both
 * `AddSink` and `RemoveSink`, which the base leaves inherited, and in the `MsgSink` table it
 * replaces `HandleMessage`.
 *
 * Recovery is partial. Its own members are not enumerated, beyond the object at `+0xa8` that the
 * destructor releases by calling a virtual on it rather than by freeing it directly.
 *
 * A slot whose verb is unrecovered keeps its table index as its title, because the index is part
 * of the layout.
 */
class LocalPlayer : public Player {
public:
    /** @ghidraAddress 0x0011e348 */
    virtual ~LocalPlayer();

    /** @ghidraAddress 0x00121ea0 */
    virtual int Slot2();

    /** @ghidraAddress 0x00121e90 */
    virtual int Slot4();

    /** @ghidraAddress 0x00121e98 */
    virtual int Slot5();

    /** @ghidraAddress 0x00122890 */
    virtual int Slot6();

    /** @ghidraAddress 0x00121ea8 */
    virtual void Slot7();

    /** @ghidraAddress 0x00122898 */
    virtual void Slot8(int first, int second);

    /** @ghidraAddress 0x001228a8 */
    virtual int Slot9(int value);

    /** @ghidraAddress 0x00121eb0 */
    virtual int Slot10();

    /** @ghidraAddress 0x0011e4e0 */
    virtual void Slot11();

    /** @ghidraAddress 0x001228c8 */
    virtual void Slot12();

    /** @ghidraAddress 0x00121ec0 */
    virtual int Slot14();

    /** @ghidraAddress 0x00121ed0 */
    virtual int Slot15();

    /** @ghidraAddress 0x00122ca0 */
    virtual int Slot16(int value);

    /** @ghidraAddress 0x00121ee0 */
    virtual int Slot17();

    /** @ghidraAddress 0x00122cc8 */
    virtual float Slot18();

    /** @ghidraAddress 0x00121ee8 */
    virtual int Slot19();

    /** @ghidraAddress 0x00122c00 */
    virtual int Slot20(int value);

    /**
     * Slot 21. Declared by this class rather than inherited.
     *
     * @ghidraAddress 0x0011e810
     */
    virtual void Slot21();

    /**
     * Slot 22. Declared by this class rather than inherited.
     *
     * @ghidraAddress 0x0011e908
     */
    virtual void Slot22();

    /** @ghidraAddress 0x0011ed98 */
    virtual void HandleMessage(Message *pMsg);

    /** @ghidraAddress 0x001228f8 */
    virtual void AddSink(MsgSink *pSink);

    /** @ghidraAddress 0x00122968 */
    virtual void RemoveSink(MsgSink *pSink);
};
