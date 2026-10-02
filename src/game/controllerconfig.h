#pragma once

#include <vector>

#include "stream/ibstream.h"
#include "stream/obstream.h"

/**
 * One controller's mapping from gameplay actions to buttons.
 *
 * The class is not polymorphic and emits no RTTI descriptor. The name is inferred from the labels
 * `Controller Config 1` through `Controller Config 4` that GlobalSettings::Print() writes for the
 * four instances GlobalSettings embeds. The object is the 12 bytes of its one vector, which the
 * GlobalSettings layout fixes. The destructor is compiler-generated and inline.
 *
 * mButtons has one entry per action slot. The configuration screen addresses the mapping by row
 * and by button index, and ActionSlot(), ButtonCode(), and ButtonIndex() translate between those
 * and the stored slot and button codes. Codes 100 through 103 are the two analog sticks' axes, and
 * codes 11 and 12 are the two stick presses.
 *
 * The translation unit also holds three uncalled template library emissions at `0x00164150`,
 * `0x001643c8`, and `0x001647e8`, which are not declared.
 */
class ControllerConfig {
public:
    /** The action slots, in mButtons order. */
    enum Slot {
        kSlotRotateRight = 0,    /*!< `rotR`. */
        kSlotRotateLeft = 1,     /*!< `rotL`. */
        kSlotAdvance = 2,        /*!< `advn`. */
        kSlotLoop = 3,           /*!< `loop`. */
        kSlotPitchRiffFirst = 4, /*!< The first of six `rpch` slots, two per riff. */
        kSlotErase = 10,         /*!< `eras`. */
        kSlotAxisFX = 11,        /*!< `axfx`. */
        kSlotAxisRegister = 12,  /*!< `regi`. */
        kSlotAxisX = 13,         /*!< `powx`. */
        kSlotAxisY = 14,         /*!< `powy`. */
        kSlotButtonPow = 15,     /*!< `powb`. */
        kSlotPlayback = 16,      /*!< `pbck`. */
        kSlotCount = 17,         /*!< The number of slots. */
    };

    /**
     * Start with the default mapping.
     *
     * @ghidraAddress 0x00163a88
     */
    ControllerConfig();

    /**
     * Assign one button, by index, to the action one configuration row controls.
     *
     * Button index 8 or 9 selects a stick rather than a button. Choosing one for the effect row
     * or the register row places that stick on the effect and register slots and the other stick
     * on the power axes, with the matching stick press on kSlotButtonPow. Choosing one for either
     * power-axis row does the same with the sticks swapped. A stick chosen for any other row
     * changes nothing.
     *
     * @param nRow The configuration row, 0 through 8.
     * @param nButtonIndex The button index, 0 through 9.
     * @ghidraAddress 0x00163c28
     */
    void SetButton(int nRow, int nButtonIndex);

    /**
     * Report the four-character action code of one slot.
     *
     * InputMap::Rebuild() calls it. The body does not read this object.
     *
     * @param nSlot The slot.
     * @return The action code, or zero for a slot out of range.
     * @ghidraAddress 0x00163d50
     */
    int ActionCode(int nSlot);

    /**
     * Read the mapping back.
     *
     * A record older than version 4 is read into a discarded vector, and the mapping keeps its
     * current contents.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x00163e10
     */
    void Load(IBStream &stream);

    /**
     * Report the button index one configuration row is mapped to.
     *
     * @param nRow The configuration row, 0 through 8.
     * @return The button index, or -1 when the stored code has none.
     * @ghidraAddress 0x00164b00
     */
    int GetButtonIndex(int nRow);

    /**
     * Translate a button index into its stored button code.
     *
     * @param nButtonIndex The button index, 0 through 9.
     * @return The code, 500 or 501 for the two stick choices, or -1 out of range.
     * @ghidraAddress 0x00164b40
     */
    int ButtonCode(int nButtonIndex);

    /**
     * Translate a stored button code into its button index.
     *
     * Both axes of a stick report that stick's index.
     *
     * @param nCode The code.
     * @return The button index, or -1 for a code with none.
     * @ghidraAddress 0x00164bc0
     */
    int ButtonIndex(int nCode);

    /**
     * Translate a configuration row into the slot it controls.
     *
     * @param nRow The configuration row, 0 through 8.
     * @return The slot, or -1 out of range.
     * @ghidraAddress 0x00164c40
     */
    int ActionSlot(int nRow);

    /**
     * Report which riff a pitch-riff slot belongs to.
     *
     * InputMap::Rebuild() calls it.
     *
     * @param nSlot The slot.
     * @return 0, 1, or 2 for the three pairs of pitch-riff slots, and 0 for every other slot.
     * @ghidraAddress 0x00164cb8
     */
    int RiffIndex(int nSlot);

    /**
     * Write the record version 4, the slot count, and every button code.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x00164d00
     */
    void Save(OBStream &stream);

    /**
     * Copy another mapping.
     *
     * @param other The mapping to copy.
     * @return This mapping.
     * @ghidraAddress 0x00164dd0
     */
    ControllerConfig &operator=(const ControllerConfig &other);

    /** The button code of each slot. Public because InputMap::Rebuild() walks it. +0x00 */
    std::vector<int> mButtons;
};
