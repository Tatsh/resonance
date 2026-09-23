#pragma once

#include <list>
#include <map>
#include <vector>

#include "app/msgsink.h"
#include "app/msgsource.h"

class Globals;
class Message;
class Player;

/**
 * Translator from controller readings to the players of one game world.
 *
 * `8InputMap` in the RTTI descriptor, with MsgSource at offset 0 and MsgSink at `+0x14`. Its two
 * tables are at `0x007cf6c0` and `0x007cf698`, the second adjusting `this` by `-20`. GrooveWorld
 * creates the one instance with a 0x60-byte allocation and the constructor at `0x00119160`, which
 * receives the application and the address of the world's player vector. The instance records
 * itself in g_pInputMap.
 *
 * Each Binding ties one player slot to one action. mBindingMap keys every physical control, built
 * by MakeKey() from a device, a port, and a button, to the binding it drives. A RawControllerMsg
 * is looked up there and turned into the message its binding's action names. The members are
 * declared in recovered offset order.
 */
class InputMap : public MsgSource, public MsgSink {
public:
    /** Actions other classes enable and disable, as four-character codes. */
    enum Action {
        kActionRotateLeft = 0x726f744c,  /*!< `rotL`, which sends RotLeftMsg. */
        kActionRotateRight = 0x726f7452, /*!< `rotR`, which sends RotRightMsg. */
    };

    /**
     * One player slot's use of one action.
     *
     * A 0x14-byte element of mBindings. The name is inferred.
     */
    struct Binding {
        /**
         * Release the axis state.
         *
         * @ghidraAddress 0x0011d958
         */
        ~Binding() {
            delete mState;
        }

        int mSlot;    /*!< The player slot the binding drives. */
        int mAction;  /*!< The action, a four-character code. */
        int mExtra;   /*!< The action's configured argument. */
        int mEnabled; /*!< Non-zero while the binding acts. */
        int *mState;  /*!< The last reported step of an axis action, or null. */
    };

    /**
     * Build the map with no binding and record it in g_pInputMap.
     *
     * @param pGlobals The application globals.
     * @param pPlayers The world's player vector.
     * @ghidraAddress 0x00119160
     */
    InputMap(Globals *pGlobals, std::vector<Player *> *pPlayers);

    /**
     * Clear g_pInputMap and release every binding.
     *
     * @ghidraAddress 0x001193d0
     */
    virtual ~InputMap();

    /**
     * Pass a RawControllerMsg to OnControllerReading() and ignore every other message.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x0011dc20
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * Report the map that exists.
     *
     * @return g_pInputMap.
     * @ghidraAddress 0x0011d9a0
     */
    static InputMap *shared();

    /**
     * Pack a device, a port, and a button into a key of mBindingMap.
     *
     * @param nDevice The device, a four-character code.
     * @param nPort The one-based port.
     * @param nButton The button.
     * @return `((nDevice << 5 | nPort) << 16) | nButton`.
     * @ghidraAddress 0x0011dc08
     */
    static int MakeKey(int nDevice, int nPort, int nButton);

    /**
     * Clear the enable word of every binding.
     *
     * @ghidraAddress 0x0011db78
     */
    void DisableEntries();

    /**
     * Set the enable word of every binding.
     *
     * @ghidraAddress 0x0011dbc0
     */
    void EnableEntries();

    /**
     * Set the enable word of every binding of one slot and one action.
     *
     * The title is inferred.
     *
     * @param nSlot The player slot.
     * @param nAction The action, a four-character code.
     * @param nEnabled Non-zero to enable.
     * @ghidraAddress 0x0011db18
     */
    void SetEnabled(int nSlot, int nAction, int nEnabled);

    /**
     * Bind one physical control to one slot's action.
     *
     * Reuses a binding of the same slot, action, and argument when one exists.
     *
     * @param nDevice The device, a four-character code.
     * @param nPort The one-based port.
     * @param nButton The button.
     * @param nSlot The player slot.
     * @param nAction The action, a four-character code.
     * @param nExtra The action's configured argument.
     * @ghidraAddress 0x0011a0f0
     */
    void AddBinding(int nDevice, int nPort, int nButton, int nSlot, int nAction, int nExtra);

    /**
     * Rebuild mBindingMap from the controller configuration.
     *
     * Binds every configured button of each of the four ports to that port's slot, then passes the
     * configuration's force-feedback setting to the world's ForceFeedbackMgr.
     * GameManagerImpl::OnUnpauseGameSystem() and GrooveWorld call it. The body is not written,
     * because the configuration's class is unrecovered. The title is inferred.
     *
     * @ghidraAddress 0x0011a230
     */
    void Rebuild();

    /**
     * Stop the three riffs of every player at the current song position.
     *
     * Sends one StopRiffMsg per player and riff, then clears mUnknown30. The title is inferred.
     *
     * @ghidraAddress 0x00119dd0
     */
    void StopAllRiffs();

private:
    // The two axis actions, `powx` and `powy`, whose bindings keep an axis state.
    enum {
        kActionAxisX = 0x706f7778,
        kActionAxisY = 0x706f7779,
    };

    // The player slots and the riffs per slot mUnknown30 covers.
    enum {
        kSlotCount = 4,
        kRiffCount = 3,
    };

    // 0x00119518. Turns one reading into the message its binding's action names. Not written,
    // because most of those messages have no constructor yet.
    void OnControllerReading(Message *pMsg);

    // 0x0011da68. Sends a StopRiffMsg. Not written, for the same reason.
    void SendStopRiff(int nArg1, Player *pPlayer, int nArg3, int nArg4);

    // 0x0011d9b0. Sends a PitchRiffMsg after a Player::Slot2() call whose result it discards.
    // Not written, for the same reason.
    void SendPitchRiff(int nArg1, Player *pPlayer, int nArg3, int nArg4);

    // 0x00119f58. The binding equal to binding in slot, action, and argument, appended with a
    // fresh axis state for an axis action when none exists.
    std::list<Binding>::iterator FindOrAddBinding(const Binding &binding);

    // 0x0011d1a0. Empties mBindingMap. The title is inferred.
    void ClearBindingMap();

    Globals *mGlobals;
    std::list<Binding> mBindings;
    std::map<int, std::list<Binding>::iterator> mBindingMap;
    std::vector<Player *> *mPlayers;
    // One word per slot and riff. The constructor and StopAllRiffs() zero every word.
    int mUnknown30[kSlotCount][kRiffCount];
};

/**
 * The map that exists, or null.
 *
 * @ghidraAddress 0x0066b6b0
 */
extern InputMap *g_pInputMap;
