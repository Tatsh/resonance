#pragma once

#include <vector>

#include "os/hxstr.h"

class MetPersonaData;

namespace Rnd {
class Cam;
class Mesh;
class Text;
} // namespace Rnd

/**
 * Scroller behind MetCreditsScreen that shows each credit while it crosses the screen.
 *
 * The class is not polymorphic and emits no RTTI, and no literal names it, so the title is
 * inferred. MetCreditsScreen builds one in its container-view slot as
 * `CreditsRoll("cpic_", "ctxt_", credit_cam, 1)` and keeps it at `+0x8c`. The object is 0x64
 * bytes.
 *
 * A credit is a numbered pair of objects, a picture mesh `cpic_NNN` and a text `ctxt_NNN`, either
 * of which may be missing. Update() walks the credits from the first one still on screen and
 * shows each credit whose extent reaches the visible band. When a credit that carries a persona
 * name first becomes visible, that persona's avatar is rendered into one of the four burn
 * textures and the texture is placed on the picture.
 *
 * The containers are sized by the number of credits but indexed by credit number, which starts at
 * the first index rather than zero. That is how the image indexes them.
 */
class CreditsRoll {
public:
    /**
     * Record the naming scheme and the camera, and reset the scroll.
     *
     * @param picturePrefix The prefix of each picture mesh name.
     * @param textPrefix The prefix of each text name.
     * @param pCam The camera the credits are projected through.
     * @param nFirstIndex The number of the first credit.
     * @ghidraAddress 0x00164df8
     */
    CreditsRoll(const HxStr &picturePrefix,
                const HxStr &textPrefix,
                Rnd::Cam *pCam,
                int nFirstIndex);

    /**
     * Rewind to the first credit, forget which personas were drawn, and fetch the identity list.
     *
     * @ghidraAddress 0x00165110
     */
    void Reset();

    /**
     * Show the credits in the visible band and hide the ones that have left it.
     *
     * Credits above the band are hidden and the scroll's starting point moves past them. The walk
     * stops at the first credit below the band.
     *
     * @return One while a credit remains below the band, and zero once the credits run out.
     * @ghidraAddress 0x00165268
     */
    int Update();

    /**
     * Hide every picture and every text.
     *
     * @ghidraAddress 0x00165b58
     */
    void HideAll();

    /**
     * Find the credits in the render manager and assign the persona names.
     *
     * Numbers from the first index are tried until neither a picture nor a text of that number
     * exists. Missing members of a pair are stored as null.
     *
     * @ghidraAddress 0x00165c38
     */
    void Build();

private:
    // Where a credit lies against the visible band. The values are bits, and the classifiers
    // combine them.
    enum Band {
        kBandAbove = 1,
        kBandVisible = 2,
        kBandBelow = 4,
        kBandAbsent = 8,
    };

    // The four persona burn textures are used in turn.
    static constexpr int kBurnSlotCount = 4;

    // 0x00165658
    int ClassifyPicture(Rnd::Mesh *pPicture);
    // 0x001658c8
    int ClassifyText(Rnd::Text *pText);
    // 0x00169970
    // The combined band of one credit.
    int Classify(Rnd::Mesh *pPicture, Rnd::Text *pText);
    // 0x00169900
    Rnd::Mesh *GetPicture(int nIndex);
    // 0x00169938
    Rnd::Text *GetText(int nIndex);
    // 0x001699e8
    // The receiver is not read.
    void SetShowing(Rnd::Mesh *pPicture, Rnd::Text *pText, int nShowing);
    // 0x00169860
    // Update() expands the same search inline.
    static inline MetPersonaData *FindPersona(const HxStr &name,
                                              const std::vector<MetPersonaData *> &identities);

    HxStr picturePrefix_;
    HxStr textPrefix_;
    Rnd::Cam *cam_;
    int firstVisible_; // The first credit Update() examines.
    int firstIndex_;
    std::vector<Rnd::Mesh *> pictures_;
    std::vector<Rnd::Text *> texts_;
    std::vector<HxStr> personaNames_; // Empty for a credit without a persona.
    std::vector<bool> burned_;        // Set once a credit's persona has been drawn.
    std::vector<MetPersonaData *> *identities_;
    int burnSlot_; // The burn texture the next persona is drawn into.
};
