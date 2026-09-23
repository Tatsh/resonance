#pragma once

#include <vector>

#include "met/metscreen.h"
#include "met/texturepairrecord.h"

struct MetRemixRecord;
namespace Rnd {
class Mat;
class Mesh;
class Text;
} // namespace Rnd

/**
 * Panel that shows the stored data of one remix.
 *
 * `18MetRemixDataScreen` in the RTTI descriptor at `0x00901c00`, with MetScreen as its one public
 * non-virtual base at offset 0. The object is 0x12c bytes and the 39-entry vtable is at
 * `0x00806a38`, the same length as the MetScreen table, so the class declares no virtual of its
 * own.
 *
 * The panel shows the song name, the recording date, the song's logo and picture, and up to four
 * player rows of a name, a persona picture, and a mesh. MetRemixDelScreen and MetRemixLoadScreen
 * fill it through ShowRecord() and hide it through SetRecordShowing().
 */
class MetRemixDataScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * The screen name is `mcdat`, the directory `metagame/Shared`, and the container
     * `memcard_remix_data`. The logo textures are `gSongLogo1.tex` and `gSongLogo2.tex`, and the
     * picture textures `gSongLabel1.tex` and `gSongLabel2.tex`.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00344740
     */
    MetRemixDataScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x00344bd0
     */
    virtual ~MetRemixDataScreen();

    /**
     * Build the screen on the heap.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x00349998
     */
    static MetRemixDataScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Fill the panel from one remix record.
     *
     * Nothing happens before the views are resolved. The panel is shown first. A record made on
     * this disc (its unknown34_ equal to GetAlbumJukeboxValue()) shows the song name from
     * configuration code 0x325, or the shorter code 0x327 when the name is wider than the text
     * wraps at, and starts loading the song's logo and picture. Any other record shows
     * `remix_unavail_disc` from code 0x258 instead and hides both textures. The date comes from
     * unknown18_, and one row per appearance takes the name and the persona burn texture, with
     * the remaining rows hidden and emptied. No view is tested for null.
     *
     * @param pRecord The record to show.
     * @ghidraAddress 0x003455a8
     */
    void ShowRecord(MetRemixRecord *pRecord);

    /**
     * Show or hide the song name, the date, the two meshes, and the four player rows.
     *
     * The two meshes are looked up by name again rather than read from the members. The name is
     * inferred.
     *
     * @param nShowing Non-zero to show.
     * @ghidraAddress 0x00345ba0
     */
    void SetRecordShowing(int nShowing);

    /**
     * Hide the panel and the unavailable notice, invalidate both textures, then enter.
     *
     * Slot 5.
     *
     * @ghidraAddress 0x00349a20
     */
    virtual void EnterAndShow();

    /**
     * Swap in each texture whose load has finished, showing its mesh only while it has one.
     *
     * Slot 26.
     *
     * @param flTime The renderer time, which the body does not read.
     * @ghidraAddress 0x00345de8
     */
    virtual void OnUnknownSlot26(float flTime);

    /**
     * Hide the unavailable notice.
     *
     * Slot 36.
     *
     * @ghidraAddress 0x00349ab8
     */
    virtual void OnUnknownSlot36();

    /**
     * Resolve the container views, the seven single views, and the four player rows.
     *
     * Slot 38. The unavailable notice is hidden, and each row's `mcrl_name_0N.txt` is emptied.
     * No view is tested for null.
     *
     * @ghidraAddress 0x00344d70
     */
    virtual void ResolveContainerViews();

private:
    std::vector<Rnd::Text *> mUnknown8c; // The player names, `mcrl_name_0N.txt`.
    std::vector<Rnd::Mat *> mUnknown98;  // The persona pictures, `mcrl_playerN.mat`.
    std::vector<Rnd::Mesh *> mUnknowna4; // The player meshes, `mcrl_freq_0N.mesh`.
    Rnd::Text *mUnknownb0;               // `mcrl_songtitle.txt`
    Rnd::Text *mUnknownb4;               // `mcrl_dob.txt`
    Rnd::Mat *mUnknownb8;                // `mcrl_photo.mat`, which takes the picture
    Rnd::Mat *mUnknownbc;                // `mcrl_logo.mat`, which takes the logo
    Rnd::Text *mUnknownc0;               // `mcrl_remixunavail.txt`
    Rnd::Mesh *mUnknownc4;               // `mcrl_label.mesh`, shown with the picture
    Rnd::Mesh *mUnknownc8;               // `mcrl_logo.mesh`, shown with the logo
    TexturePairRecord mLogoTextures;
    TexturePairRecord mLabelTextures;
};
